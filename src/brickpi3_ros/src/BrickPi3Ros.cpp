//
// Created by hl on 2/10/19.
//

#include <string>
#include <iostream>
#include <Scheduler.h>
#include <boost/bind.hpp>
#include <ros/ros.h>
#include <ros/console.h>
#include "BrickPi3Motor.h"
#include "BrickPi3Ultrasonic.h"
#include "BrickPi3Color.h"
using namespace std;


class BrickPi3Ros {
private:
    Bosma::Scheduler scheduler;
    ros::NodeHandle nodeHandle;

public:
    BrickPi3Ros(int argc, char** argv);
    virtual ~BrickPi3Ros() {}
    void run() {ros::spin();}
};

BrickPi3Ros::BrickPi3Ros(int argc, char** argv):
    scheduler(8) // BrickPi3 supports 8 devvices.
{
    // Parse the ros_robot.yaml file and create the appropriate sensors and actuators
    XmlRpc::XmlRpcValue brickpi3_robot;
    nodeHandle.getParam("/brickpi3_ros/brickpi3_robot", brickpi3_robot);
    ROS_ASSERT(brickpi3_robot.getType() == XmlRpc::XmlRpcValue::TypeArray);
    cout << "---------------------------------" << endl;
    for (int i = 0; i < brickpi3_robot.size(); i++) {
        XmlRpc::XmlRpcValue brickpi3_device = brickpi3_robot[i];
        string type = brickpi3_device["type"];
        if (type == "motor") {
            string name = brickpi3_device["name"];
            string port = brickpi3_device["port"];
            double freq = brickpi3_device["frequency"];

            auto motor = new BrickPi3Motor(name, port, freq); //todo: Needs better resource handling. Should be freed when the BrickPi3Ros object is deleted.
            scheduler.every(std::chrono::milliseconds(static_cast<int >(1000.0/freq)), boost::bind(&BrickPi3Motor::schedulerCB, motor));
        }
        else if (type == "ultrasonic") {
            string name = brickpi3_device["name"];
            string frame_id = brickpi3_device["frame_id"];
            string port = brickpi3_device["port"];
            double freq = brickpi3_device["frequency"];
            double min_range = brickpi3_device["min_range"];
            double max_range = brickpi3_device["max_range"];
            double spread_angle = brickpi3_device["spread_angle"];

            auto ultrasonic = new BrickPi3Ultrasonic(name, frame_id, port, freq, min_range, max_range, spread_angle); //todo: Needs better resource handling.
            scheduler.every(std::chrono::milliseconds(static_cast<int >(1000.0/freq)), boost::bind(&BrickPi3Ultrasonic::schedulerCB, ultrasonic));
        }
        else if (type ==  "color")
        {
            string name = brickpi3_device["name"];
            string frame_id = brickpi3_device["frame_id"];
            string port = brickpi3_device["port"];
            double freq = brickpi3_device["frequency"];

            auto color = new BrickPi3Color(name, frame_id, port, freq); //todo: Needs better resource handling.
            scheduler.every(std::chrono::milliseconds(static_cast<int >(1000.0/freq)), boost::bind(&BrickPi3Color::schedulerCB, color));
        }
    }
}

int main(int argc, char** argv) {
    ros::init(argc, argv, "brickpi3_ros"); // Name of this node.
    BrickPi3Ros brickPi3Ros(argc, argv);
    brickPi3Ros.run();
}
