#include "arucoDetection.h"

using namespace ad;

arucoDetection::arucoDetection() {

    MarkerPose.header.frame_id = "camera_link";

    ROS_INFO_STREAM("Started Aruco Detection Node");
    pub_marker = nh.advertise < sensor_msgs::Image > ("/marker_detect/image_raw", 100);					// Publisher for Image of detection
    pub_pose_estimated = nh.advertise < sensor_msgs::Image>("/marker_detect/estimated",100);				// Publisher for Image of estimation
    pub_marker_id = nh.advertise < std_msgs::String > ("/marker_detect/id", 100);						// Publisher for ID of detected marker
    pub_pose = nh.advertise < geometry_msgs::PoseStamped > ("marker/pose", 1000);						// Publisher for pose of detected marker
    pub_velocity = nh.advertise < geometry_msgs::Twist > ("/cmd_vel", 1);							// Publisher for rover velocity
    
    sub_camera = nh.subscribe("/camera/color/image_raw", 100, & arucoDetection::imageCallback, this);			// Callback for Image from camera - Realsense d435
    //sub_camera_sim = nh->subscribe("/color_cam/color/image_raw", 100, &arucoDetection::imageCallback1,this);		// General Callback for testing
}

void arucoDetection::imageCallback(const sensor_msgs::ImageConstPtr & msg) {

    camera_image = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
    
    arucoDetection::detect_aruco();
    
    estimate_pose();
    
    msg_pub = camera_image -> toImageMsg();
    pub_marker.publish(msg_pub);
    
};

void arucoDetection::detect_aruco() {

    //set_params();
    
    markers_drawn_img = camera_image -> image;
    cv::aruco::detectMarkers(markers_drawn_img, dictionary, corners, marker_ids_detected, params);
    
    marker_ID.data = " ";
    
    for (int i = 0; i < marker_ids_detected.size(); i++) {
    
        ID = std::to_string(marker_ids_detected[i]);
        marker_ID.data.append(ID);
          
    }
    
    pub_marker_id.publish(marker_ID);
    
    if (marker_ids_detected.size() > 0) {
    
        cv::aruco::drawDetectedMarkers(markers_drawn_img, corners, marker_ids_detected);
    }
};

void arucoDetection::set_params() {

    params -> cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
    params -> adaptiveThreshWinSizeMin = 5;
    params -> adaptiveThreshWinSizeMax = 20;
    params -> adaptiveThreshWinSizeStep = 5;
    params -> adaptiveThreshConstant = 25;
    params -> minMarkerPerimeterRate = 0.03;
    params -> maxMarkerPerimeterRate = 1;
    params -> polygonalApproxAccuracyRate = 0.052;
    params -> minCornerDistanceRate = 0.05;
    params -> minMarkerDistanceRate = 0.1;
    params -> minDistanceToBorder = 2;
    params -> cornerRefinementWinSize = 4;
    params -> cornerRefinementMinAccuracy = 0.1;
    params -> cornerRefinementMaxIterations = 50;
    params->minOtsuStdDev = 0.1;

};

void arucoDetection::estimate_pose() {
    final_image = camera_image -> image;
    cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    cameraMatrix.at < double > (0, 0) = 2.3396381685789738e+03;
    cameraMatrix.at < double > (0, 2) = 960.;
    cameraMatrix.at < double > (1, 1) = 2.3396381685789738e+03;
    cameraMatrix.at < double > (1, 2) = 540.;
    
    distortionCoeffs = cv::Mat::zeros(8, 1, CV_64F);
    distortionCoeffs.at < double > (0, 0) = -1.0982746232841779e-01;
    distortionCoeffs.at < double > (1, 0) = 2.2689585715220828e-01;
    distortionCoeffs.at < double > (2, 0) = 0.;
    distortionCoeffs.at < double > (3, 0) = 0.;
    distortionCoeffs.at < double > (4, 0) = -2.2112148171171589e-01;

        if (marker_ids_detected.size() > 0) {
        
            cv::aruco::estimatePoseSingleMarkers(corners, 0.02, cameraMatrix, distortionCoeffs, RotationalVectors, TranslationalVectors);
            
            try {
            
                cv::aruco::drawAxis(final_image, cameraMatrix, distortionCoeffs, RotationalVectors[0], TranslationalVectors[0], 0.05);
            } 
            
            catch (...) {
                ROS_INFO_STREAM("\n Unable to draw axis.");
            }
            
            msg_pub_estimated = camera_image -> toImageMsg();
            pub_pose_estimated.publish(msg_pub_estimated);
            
            
            calculate_pose();
            

    }
};

void arucoDetection::calculate_pose() {

    for (int i = 0; i < TranslationalVectors.size(); ++i) {
        cv::Vec3d curr_RotVec;	
        cv::Vec3d curr_TransVec;
        curr_RotVec = RotationalVectors[i];
        curr_TransVec = TranslationalVectors[i];
        cv::Rodrigues(curr_RotVec, rotMat);
        if (cv::determinant(rotMat) > 0.99 && cv::determinant(rotMat) < 1.01) {
        
            origin = rotMat * -curr_TransVec;
            
            tf2::Quaternion q;
            q.setRPY(curr_RotVec[0], curr_RotVec[1], curr_RotVec[2]);
            q = q.normalize();
            
            geometry_msgs::Quaternion quaternion = tf2::toMsg(q);
            MarkerPose.pose.position.x = curr_TransVec[2];
            MarkerPose.pose.position.y = curr_TransVec[0];
            MarkerPose.pose.position.z = -curr_TransVec[1];
            MarkerPose.pose.orientation = quaternion;
            nh.setParam("/marker_ids_detected", marker_ids_detected[0]);
            nh.setParam("/marker/x", curr_TransVec[2]);
            nh.setParam("/marker/y", curr_TransVec[0]);
            nh.setParam("/marker/z", curr_TransVec[1]);
            
            double dist = sqrt((curr_TransVec[0] * curr_TransVec[0]) + (curr_TransVec[1] * curr_TransVec[1]) + (curr_TransVec[2] * curr_TransVec[2]));
            ROS_INFO_STREAM("Marker Distance : " << dist);
            nh.setParam("/marker/distance", dist);
            
            if (marker_ids_detected.size() > 0) {
            
                times_detected = times_detected + 1;
            }
            
            if (times_detected > 3) {
            
                try {
                    Vel.linear.x = 0;
                    Vel.angular.z = 0;
                    pub_velocity.publish(Vel);
                    //std::system("rosnode kill /search_pattern");
                    
                } 
                
                catch (...) {
                
                    ROS_INFO_STREAM("Search Pattern not running");
                }

            }
            pub_pose.publish(MarkerPose);
        }

    }
    
    /*
    Function for camera calliberation. Uncomment if needed only :
    
    void Aruco_Detection::create_checkboard()
   {
        std::cout << ""<<"\n";

        for (int i=0; i<boardSize.height; i++) {
            for (int j=0; j<boardSize.width; j++) {
                obj.push_back(Point3f(i, j, 0.0f));
            }
        }
   }*/
};

int main(int argc, char ** argv) {

    ros::init(argc, argv, "aruco_detection_node");
    arucoDetection object = arucoDetection();
    ros::spin();
}
