//
// Created by hl on 2/7/19.
//

#include <string>
#include <stdio.h>
#include <Scheduler.h>
#include <boost/bind.hpp>
#include <ros/ros.h>
#include <ros/console.h>
#include <teleop/JoystickPublisher.h>
#include <teleop/Joystick.h>
#include <teleop/KeyboardPublisher.h>
#include <teleop/Keyboard.h>
#include <geometry_msgs/Twist.h>


class VelocityPublisher {
private:
    Bosma::Scheduler scheduler;
    ros::NodeHandle nodeHandle;
    ros::Publisher cmdVelPublisher;
    ros::Subscriber joystickSubscriber;
    ros::Subscriber keyboardSubscriber;
    int publishEveryMs;
    double minVelLinear;
    double maxVelLinear;
    double minVelAngular;
    double maxVelAngular;
    double deltaVelLinear;
    double deltaVelAngular;
    double currVelLinear;
    double currVelAngular;

    // Callback functions for ROS topics.
    void schedulerCB();
    void joystickCB(const teleop::Joystick& joy);
    void keyboardCB(const teleop::Keyboard& key);

public:
    VelocityPublisher(int argc, char** argv);
    virtual ~VelocityPublisher() {};
    void run() {ros::spin();}
};

VelocityPublisher::VelocityPublisher(int argc, char** argv) :
    cmdVelPublisher(nodeHandle.advertise<geometry_msgs::Twist>("/cmd_vel", 10)),
    joystickSubscriber(nodeHandle.subscribe("/joystick", 10, &VelocityPublisher::joystickCB, this)),
    keyboardSubscriber(nodeHandle.subscribe("/keyboard", 10, &VelocityPublisher::keyboardCB, this))
{
    nodeHandle.param("/velocity_publisher/publish_every_ms", publishEveryMs, 100);
    nodeHandle.param("/velocity_publisher/min_vel_linear", minVelLinear, 0.04); // m/s
    nodeHandle.param("/velocity_publisher/max_vel_linear", maxVelLinear, 0.10); // m/s
    nodeHandle.param("/velocity_publisher/min_vel_angular", minVelAngular, 0.64); // rad/s
    nodeHandle.param("/velocity_publisher/max_vel_angular", maxVelAngular, 1.60); // rad/s
    deltaVelLinear = (maxVelLinear - minVelLinear) / 10.0;
    deltaVelAngular = (maxVelAngular - minVelAngular) / 10.0;
    currVelLinear = 0.0;
    currVelAngular = 0.0;

    printf("publish_every_ms: %d\n", publishEveryMs);
    printf("min_vel_linear: %f m/s\n", minVelLinear);
    printf("max_vel_linear: %f m/s\n", maxVelLinear);
    printf("min_vel_angular: %f rad/s\n", minVelAngular);
    printf("max_vel_angular: %f rad/s\n", maxVelAngular);

    scheduler.every(std::chrono::milliseconds(publishEveryMs), boost::bind(&VelocityPublisher::schedulerCB, this));
}


void VelocityPublisher::schedulerCB()
{
    geometry_msgs::Twist vel;
    vel.linear.x = currVelLinear;
    vel.angular.z = currVelAngular;
    cmdVelPublisher.publish(vel);
}


void VelocityPublisher::joystickCB(const teleop::Joystick& joy)
{
    JoystickEvents event = static_cast<JoystickEvents>(joy.event);
    switch (event) {
        case JS_EVENT_BUTTON0_DOWN:
            currVelLinear = 0.0;
            currVelAngular = 0.0;
            break;
        case JS_EVENT_BUTTON1_DOWN:
            currVelLinear = 0.0;
            currVelAngular = 0.0;
            break;
        case JS_EVENT_AXIS_UP:
            currVelLinear += deltaVelLinear;
            if (currVelLinear > maxVelLinear) {
                currVelLinear = maxVelLinear;
            }
            else if (currVelLinear > -minVelLinear && currVelLinear < minVelLinear) {
                currVelLinear = minVelLinear;
            }
            break;
        case JS_EVENT_AXIS_DOWN:
            currVelLinear -= deltaVelLinear;
            if (currVelLinear < -maxVelLinear) {
                currVelLinear = -maxVelLinear;
            }
            else if (currVelLinear > -minVelLinear && currVelLinear < minVelLinear) {
                currVelLinear = -minVelLinear;
            }
            break;
        case JS_EVENT_AXIS_LEFT:
            currVelAngular -= deltaVelAngular;
            if (currVelAngular < -maxVelAngular) {
                currVelAngular = -maxVelAngular;
            }
            else if (currVelAngular > -minVelAngular && currVelAngular < minVelAngular) {
                currVelAngular = -minVelAngular;
            }
            break;
        case JS_EVENT_AXIS_RIGHT:
            currVelAngular += deltaVelAngular;
            if (currVelAngular > maxVelAngular) {
                currVelAngular = maxVelAngular;
            }
            else if (currVelAngular > -minVelAngular && currVelAngular < minVelAngular) {
                currVelAngular = minVelAngular;
            }
            break;
        default:
            break;
    }
}


void VelocityPublisher::keyboardCB(const teleop::Keyboard& key)
{
    KeyboardEvents event = static_cast<KeyboardEvents>(key.event);
    switch (event) {
        case KB_EVENT_UP:
            currVelLinear += deltaVelLinear;
            if (currVelLinear > maxVelLinear) {
                currVelLinear = maxVelLinear;
            }
            else if (currVelLinear > -minVelLinear && currVelLinear < minVelLinear) {
                currVelLinear = minVelLinear;
            }
            break;
        case KB_EVENT_LEFT:
            currVelAngular -= deltaVelAngular;
            if (currVelAngular < -maxVelAngular) {
                currVelAngular = -maxVelAngular;
            }
            else if (currVelAngular > -minVelAngular && currVelAngular < minVelAngular) {
                currVelAngular = -minVelAngular;
            }
            break;
        case KB_EVENT_RIGHT:
            currVelAngular += deltaVelAngular;
            if (currVelAngular > maxVelAngular) {
                currVelAngular = maxVelAngular;
            }
            else if (currVelAngular > -minVelAngular && currVelAngular < minVelAngular) {
                currVelAngular = minVelAngular;
            }
            break;
        case KB_EVENT_DOWN:
            currVelLinear -= deltaVelLinear;
            if (currVelLinear < -maxVelLinear) {
                currVelLinear = -maxVelLinear;
            }
            else if (currVelLinear > -minVelLinear && currVelLinear < minVelLinear) {
                currVelLinear = -minVelLinear;
            }
            break;
        case KB_EVENT_SPACE:
            currVelLinear = 0.0;
            currVelAngular = 0.0;
            break;
        default:
            break;
    }
}


int main(int argc, char** argv) {
    ros::init(argc, argv, "velocity_publisher"); // Name of this node.
    VelocityPublisher velocityPublisher(argc, argv);
    velocityPublisher.run();
}
