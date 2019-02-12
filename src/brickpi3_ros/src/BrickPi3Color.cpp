//
// Created by hl on 2/10/19.
//

#include <string>
#include <iostream>
#include <brickpi3_ros/Color.h>
#include "BrickPi3Color.h"
using namespace std;


BrickPi3Color::BrickPi3Color(const string& aName, const string& aFrameId, const string& aPort, const double aFrequency):
    colorPublisher(nodeHandle.advertise<brickpi3_ros::Color>("/" + aName, 10))
{
    name = aName;
    frameId = aFrameId;
    port = port2id(aPort);
    freq = aFrequency;
    seqNo = 0;

    brickPi3.set_sensor_type(port, SENSOR_TYPE_NXT_COLOR_FULL);
}


void BrickPi3Color::schedulerCB()
{
    lock_guard<std::mutex> guard(mutex);

    sensor_color_t sensorColor;
    int rc = brickPi3.get_sensor(port, &sensorColor);
    if (rc == 0) {
        printf("Color sensor: detected %d red %4d green %4d blue %4d ambient %4d   ", sensorColor.color, sensorColor.reflected_red, sensorColor.reflected_green, sensorColor.reflected_blue, sensorColor.ambient);

        brickpi3_ros::Color rosColor;
        rosColor.header.frame_id = frameId;
        rosColor.header.stamp = ros::Time::now();
        rosColor.header.seq = seqNo++;
        rosColor.r = static_cast<double>(sensorColor.reflected_red);
        rosColor.g = static_cast<double>(sensorColor.reflected_green);
        rosColor.b = static_cast<double>(sensorColor.reflected_blue);
        rosColor.intensity = static_cast<double>(sensorColor.ambient);

        colorPublisher.publish(rosColor);
    }
    else {
        ROS_ERROR("BrickPi3 failed to read Color sensor %s, rc %d", name.c_str(), rc);
    }
}
