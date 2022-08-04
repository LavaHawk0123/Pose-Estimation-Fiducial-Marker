#!/usr/bin/env python3

import rospy
import cv2 as cv
import math
import cv2.aruco as aruco
import numpy as np
import glob
import numpy
from std_msgs.msg import String
from sensor_msgs.msg import Image
from nav_msgs.msg import Odometry
from cv_bridge import CvBridge, CvBridgeError
import tf
from geometry_msgs.msg import Quaternion,PoseStamped

class image_converter:

  def __init__(self):
    self.cameraMatrix = numpy.zeros((3,3))
    self.distortionCoeffs = numpy.zeros((5,0))
    self.rvec = numpy.zeros((2,0))
    self.tvec = numpy.zeros((2,0))
    self.odom = PoseStamped()
    self.image_pub = rospy.Publisher("/detected_markers",Image, queue_size=10)
    self.id_pub = rospy.Publisher("/arudo_ID", String, queue_size=10)

    self.pose_pub = rospy.Publisher("/marker/pose", PoseStamped, queue_size=10)
    self.bridge = CvBridge()
    self.image_sub = rospy.Subscriber("/zed2/zed_node/rgb/image_rect_color",Image,self.callback)
    self.cv_image = cv.imread("calib_result.jpg")
    self.image_message = Image()
    self.dit = 0.0
  
    
  def callback(self,data):
    try:
      self.cv_image = self.bridge.imgmsg_to_cv2(data, "bgr8")
    except CvBridgeError as e:
      print(e)
    self.init_params()
    self.detect_markers()

  def detect_markers(self):
    (corners, ids, rejected) = cv.aruco.detectMarkers(self.cv_image, self.arucoDict, parameters=self.arucoParams)
    cv.aruco.drawDetectedMarkers(self.cv_image,corners,ids)
    rospy.loginfo("Detected Markers {}".format(ids))
    if ids != None:
      for i in range(0, len(ids)):  # Iterate in markers
          # Estimate pose of each marker and return the values rvec and tvec---different from camera coefficients
          self.rvec, self.tvec, markerPoints = aruco.estimatePoseSingleMarkers(corners[i], 0.02, self.cameraMatrix, self.distortionCoeffs)
          aruco.drawAxis(self.cv_image, self.cameraMatrix, self.distortionCoeffs, self.rvec, self.tvec, 0.01)
          self.publish_pose()
    self.image_message = self.bridge.cv2_to_imgmsg(self.cv_image, encoding="passthrough")
    self.image_pub.publish(self.image_message)

  def publish_pose(self):
    print(self.rvec[0][0])
    #for i in range(0,self.rvec[0][0].size-1):
    current_rvec = numpy.zeros((3,0))
    current_tvec = numpy.zeros((3,0))
    rotMat = numpy.zeros((3,3))
    current_rvec = self.rvec[0][0]
    current_tvec = self.tvec[0][0]
    rotMat = cv.Rodrigues(current_rvec)
    #if cv.determinant(cv.Mat(rotMat)) > 0.99 and cv.determinant(cv.Mat(rotMat)) < 1.01:
    quaternion = tf.transformations.quaternion_from_euler(current_rvec[0], current_rvec[1], current_rvec[2])
    #origin = rotMat*-current_tvec
    self.odom.header.frame_id = "chassis"
    self.odom.pose.orientation.x = quaternion[0]
    self.odom.pose.orientation.y = quaternion[1]
    self.odom.pose.orientation.z = quaternion[2]
    self.odom.pose.orientation.w = quaternion[3]
    self.odom.pose.position.x =  current_tvec[2]
    self.odom.pose.position.y =  current_tvec[0]
    self.odom.pose.position.z = -current_tvec[1]
    self.pose_pub.publish(self.odom)
    self.dist = math.sqrt((current_tvec[0] * current_tvec[0]) + (current_tvec[1] * current_tvec[1]) + (current_tvec[2] * current_tvec[2]))
    rospy.loginfo("Marker Pose : \n")
    rospy.loginfo("x : " + str(current_tvec[2]))
    rospy.loginfo("y : " + str(current_tvec[0]))
    rospy.loginfo("z : " + str(-current_tvec[1]))
    rospy.loginfo("Marker Distance : " + str(self.dist))
    #rospy.set_param("/marker/x", current_tvec[2])
    #rospy.set_param("/marker/y", current_tvec[0])
    #rospy.set_param("/marker/z", current_tvec[1])


  def init_params(self):
    self.arucoDict = cv.aruco.Dictionary_get(cv.aruco.DICT_4X4_50)
    self.arucoParams = cv.aruco.DetectorParameters_create()
    self.cameraMatrix[0:] = [1059.59,0,1099.32]
    self.cameraMatrix[1:] = [0,1059.29,601.431]
    self.cameraMatrix[2:] = [0,0,0]
    self.distortionCoeffs[0] = -0.0430829
    self.distortionCoeffs[1] = 0.011427
    self.distortionCoeffs[2] = 4.65188e-06
    self.distortionCoeffs[3] = -0.000150164
    self.distortionCoeffs[4] = 0.00525169
    #cv.imshow(self.cv_image)
  
if __name__ == '__main__':
  rospy.init_node('Detection', anonymous=True)
  ic = image_converter()
  rospy.spin()
