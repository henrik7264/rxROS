#include <ros/ros.h>
#include <geometry_msgs/PointStamped.h>
#include <tf/transform_listener.h>

void transformPoint(const tf::TransformListener& transformListener) {
    //we'll create a point in the base_laser frame that we'd like to transform to the base_link frame
    geometry_msgs::PointStamped laser_point;
    laser_point.header.frame_id = "laser_link";

    //we'll just use the most recent transform available for our simple example
    laser_point.header.stamp = ros::Time();

    //just an arbitrary point in space
    laser_point.point.x = 1.0;
    laser_point.point.y = 0.2;
    laser_point.point.z = 0.0;

    try {
        geometry_msgs::PointStamped base_point;
        transformListener.transformPoint("base_link", laser_point, base_point);

        ROS_INFO("laser_frame: (%.2f, %.2f. %.2f) -----> base_frame: (%.2f, %.2f, %.2f) at time %.2f", laser_point.point.x, laser_point.point.y, laser_point.point.z, base_point.point.x, base_point.point.y, base_point.point.z, base_point.header.stamp.toSec());
     }
     catch(tf::TransformException& ex){
         ROS_ERROR("Received an exception trying to transform a point from \"laser_frame\" to \"base_frame\": %s", ex.what());
     }
}

int main(int argc, char** argv){
    ros::init(argc, argv, "ros_robot_tf_listener");
    ros::NodeHandle nodeHandle;
    tf::TransformListener transformListener(ros::Duration(10));

    //we'll transform a point once every second
    ros::Timer timer = nodeHandle.createTimer(ros::Duration(1.0), boost::bind(&transformPoint, boost::ref(transformListener)));

    ros::spin();
}
