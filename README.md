# Pose-Estimation-Fiducial-Marker
A Repository countaining a ROS API for detecting Fiducial Markers and estimating their pose in the camera frame written in C++ for ROS Framework

Build opencv for ROS noetic :

```
sudo apt-get update -y
sudo apt-get install -y libopencv-dev
```
Install cv-bridge : 

```
sudo apt-get install ros-$ROS_DISTRO-cv-bridge
```

Install Image Transport : 

```
sudo apt-get update -y
sudo apt-get install -y libimage-transport-dev
```
Additions to CMakeLists : 

In REQUIRED COMPONENTS : 

```
find_package(catkin REQUIRED COMPONENTS
 rospy
 roscpp
 sensor_msgs
 std_msgs
 cv_bridge
 image_transport
)
```
Include Directories : 

```
include_directories(include ${catkin_INCLUDE_DIRS})
FIND_PACKAGE( OpenCV REQUIRED )                              
INCLUDE_DIRECTORIES( ${OpenCV_INCLUDE_DIRS} )
```
Building Opencv codes in CMakeLists.txt: 

```
add_executable(<build file> src/<filename>.cpp)
target_link_libraries(<build file> ${OpenCV_LIBS} ${catkin_LIBRARIES})
```
Build the package
```
source /opt/ros/$ROS_DISTRO/setup.bash
roscd catkin_ws
catkin_make
```

Launch Aruco Detection : 

```
roslaunch aruco_detection_ros detection.launch
```

### Output : 
<br>

![Aruco_Rover](https://user-images.githubusercontent.com/75236655/183000738-ff16b3c0-1e36-4f31-bcf1-1e0582349352.gif)
<br>

![aruco_stat_comp](https://user-images.githubusercontent.com/75236655/183000753-6fd48747-6348-4eba-976b-297a02f5d776.gif)
