//
// Created by hl on 2/24/19.
//

#ifndef RXROS_H
#define RXROS_H

#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <cassert>
#include <linux/input.h>
#include <rxcpp/rx.hpp>
#include <ros/ros.h>
#include <ros/console.h>
#include <tf/transform_broadcaster.h>
#include <tf/transform_listener.h>


namespace rxros
{
    static void init(int argc, char** argv, const std::string& name) {ros::init(argc, argv, name);}
    static void spin() {ros::spin();}
    static bool ok() {return ros::ok();}


    class node: public ros::NodeHandle
    {
    private:
        node() = default;

    public:
        ~node() = default;

        template<class T>
        ros::Subscriber subscribe(const std::string& topic, uint32_t queue_size, const std::function<void(const T&)>& callback)
        {
            return ros::NodeHandle::subscribe<T>(topic, queue_size, static_cast<boost::function<void(const T&)>>(callback));
        }

        static auto get_handle() {
            static node self;
            return self;
        }
    }; // end of class node


    class exception
    {
    private:
        exception() = default;

    public:
        ~exception() = default;
        static auto system_error(const int errCode, const std::string &msg)
        {
            return std::make_exception_ptr(std::system_error(std::error_code(errCode, std::generic_category()), msg));
        }
    }; // end of class exception


    class logging: public std::ostringstream
    {
    private:
        enum LogLevel {DEBUG, INFO, WARN, ERROR, FATAL};
        LogLevel logLevel;

    public:
        logging() = default;
        ~logging() override {
            switch(logLevel) {
                case DEBUG:
                    ROS_DEBUG("%s", str().c_str());
                    break;
                case INFO:
                    ROS_INFO("%s", str().c_str());
                    break;
                case WARN:
                    ROS_WARN("%s", str().c_str());
                    break;
                case ERROR:
                    ROS_ERROR("%s", str().c_str());
                    break;
                case FATAL:
                    ROS_FATAL("%s", str().c_str());
                    break;
                default:
                    ROS_FATAL("Ups!!!!");
                    break;
            }
        }

        logging& debug()
        {
            logLevel = DEBUG;
            return *this;
        }

        logging& info()
        {
            logLevel = INFO;
            return *this;
        }

        logging& warn()
        {
            logLevel = WARN;
            return *this;
        }

        logging& error()
        {
            logLevel = ERROR;
            return *this;
        }

        logging& fatal()
        {
            logLevel = FATAL;
            return *this;
        }
    }; // end of class logging


    class parameter
    {
    private:
        parameter() = default;

    public:
        ~parameter() = default;

        template<typename T>
        static auto get(const std::string& name, const T& defaultValue)
        {
            T param;
            node::get_handle().param<T>(name, param, defaultValue);
            return param;
        }

        static auto get(const std::string& name, const int defaultValue)
        {
            int param;
            node::get_handle().param(name, param, defaultValue);
            return param;
        }

        static auto get(const std::string& name, const double defaultValue)
        {
            double param;
            node::get_handle().param(name, param, defaultValue);
            return param;
        }

        static auto get(const std::string& name, const char* defaultValue)
        {
            return get<std::string>(name, defaultValue);
        }

        static auto get(const std::string& name, const std::string& defaultValue)
        {
            return get<std::string>(name, defaultValue);
        }
    }; // end of class Parameter


    class observable
    {
    private:
        observable() = default;

    public:
        ~observable() = default;

        template<class T>
        static auto from_topic(const std::string& topic, const uint32_t queueSize = 10)
        {
            auto observable = rxcpp::observable<>::create<T>(
                [=](rxcpp::subscriber<T> subscriber) {
                    auto callback = [=](const T& val){subscriber.on_next(val);};
                    ros::Subscriber rosSubscriber(node::get_handle().subscribe<T>(topic, queueSize, callback));
                    ros::waitForShutdown();
                    subscriber.on_completed();});
            return observable.subscribe_on(rxcpp::synchronize_new_thread());
        }

        static auto from_transform_listener(const std::string& frameId, const std::string& childFrameId, const double frequencyInHz = 10.0)
        {
            return rxcpp::observable<>::create<tf::StampedTransform>(
                [=](rxcpp::subscriber<tf::StampedTransform> subscriber) {
                    tf::TransformListener listener;
                    ros::Rate rate(frequencyInHz);
                    bool errReported = false;
                    while (rxros::ok()) {
                        try {
                            tf::StampedTransform transform;
                            listener.lookupTransform(frameId, childFrameId, ros::Time(0), transform);
                            subscriber.on_next(transform);
                        }
                        catch (...) {
                            std::exception_ptr err = std::current_exception();
                            subscriber.on_error(err);
                            errReported = true;
                            break;
                        }
                        rate.sleep();
                    }
                    if (!errReported) {
                        subscriber.on_completed();
                    }});
        }

        template<class T>
        static auto from_device(const std::string& deviceName)
        {
            return rxcpp::observable<>::create<T>(
                [=](rxcpp::subscriber<T> subscriber) {
                    int fd = open(deviceName.c_str(), O_RDONLY | O_NONBLOCK);
                    if (fd < 0)
                        subscriber.on_error(rxros::exception::system_error(errno, std::string("Cannot open device ") + deviceName));
                    else {
                        fd_set readfds; // initialize file descriptor set.
                        FD_ZERO(&readfds);
                        FD_SET(fd, &readfds);
                        T event{};
                        bool errReported = false;
                        while (rxros::ok()) {
                            int rc = select(fd + 1, &readfds, nullptr, nullptr, nullptr);  // wait for input on keyboard device
                            if (rc == -1 &&
                                errno == EINTR) { // select was interrupted. This is not an error but we exit the loop
                                subscriber.on_completed();
                                close(fd);
                                break;
                            } else if (rc == -1 || rc == 0) { // select failed and we issue an error.
                                subscriber.on_error(rxros::exception::system_error(errno, std::string("Failed to read device ") + deviceName));
                                close(fd);
                                errReported = true;
                                break;
                            } else if (FD_ISSET(fd, &readfds)) {
                                ssize_t sz = read(fd, &event, sizeof(T)); // read element from device.
                                if (sz == -1) {
                                    subscriber.on_error(rxros::exception::system_error(errno, std::string("Failed to read device ") + deviceName));
                                    close(fd);
                                    errReported = true;
                                    break;
                                } else if (sz == sizeof(T)) {
                                    subscriber.on_next(event); // populate the event on the
                                }
                            }
                        }
                        if (!errReported) {
                            subscriber.on_completed();
                        }
                    }
                });
        }

        // Parse the robot.yaml file and create an observable stream for the configuration of the sensors and actuators
        static auto from_yaml(const std::string& aNamespace)
        {
            // Support class to simplify access to configuration parameters for device
            class DeviceConfig
            {
            private:
                std::string type;
                std::string name;
                std::string frameId;
                std::string port;
                double frequency;
                double minRange;
                double maxRange;
                double spreadAngle;

            public:
                DeviceConfig(XmlRpc::XmlRpcValue& value) {
                    type = value.hasMember("type") ? (std::string) value["type"] : std::string("");
                    name = value.hasMember("name") ? (std::string) value["name"] : std::string("");
                    frameId = value.hasMember("frame_id") ? (std::string) value["frame_id"] : std::string("");
                    port = value.hasMember("port") ? (std::string) value["port"] : std::string("");
                    frequency = value.hasMember("frequency") ? (double) value["frequency"] : 0.0;
                    minRange = value.hasMember("min_range") ? (double) value["min_range"] : 0.0;
                    maxRange = value.hasMember("max_range") ? (double) value["max_range"] : 0.0;
                    spreadAngle = value.hasMember("spread_angle") ? (double) value["spread_angle"] : 0.0;
                }
                ~DeviceConfig() = default;

                const std::string& getType() const {return type;}
                const std::string& getName() const {return name;}
                const std::string& getFrameId() const {return frameId;}
                const std::string& getPort() const {return port;}
                double getFrequency() const {return frequency;}
                double getMinRange() const { return minRange;}
                double getMaxRange() const { return  maxRange;}
                double getSpreadAngle() const { return spreadAngle;}
            };

            return rxcpp::observable<>::create<DeviceConfig>(
                [=](rxcpp::subscriber<DeviceConfig> subscriber) {
                    XmlRpc::XmlRpcValue robotConfig;
                    node::get_handle().getParam(aNamespace, robotConfig);
                    assert (robotConfig.getType() == XmlRpc::XmlRpcValue::TypeArray);
                    for (int i = 0; i < robotConfig.size(); i++) {
                        DeviceConfig deviceConfig(robotConfig[i]);
                        subscriber.on_next(deviceConfig);
                    }
                    subscriber.on_completed();
                });
        }
    }; // end of class observable
} // end of namespace rxros


namespace rxros
{
    namespace operators
    {
        auto sample_with_frequency(const double frequency) {
            return [=](auto&& source) {
                const std::chrono::milliseconds durationInMs(static_cast<long>(1000.0/frequency));
                return rxcpp::observable<>::interval(durationInMs).with_latest_from(
                    [=](const auto intervalObsrv, const auto sourceObsrv) { return sourceObsrv; }, source);};}


        template<class Coordination>
        auto sample_with_frequency(const double frequency, Coordination coordination) {
            return [=](auto&& source) {
                const std::chrono::milliseconds durationInMs(static_cast<long>(1000.0/frequency));
                return rxcpp::observable<>::interval(durationInMs, coordination).with_latest_from(
                    [=](const auto intervalObsrv, const auto sourceObsrv) { return sourceObsrv; }, source);};}


        template<typename T>
        auto publish_to_topic(const std::string &topic, const uint32_t queue_size = 10) {
            return [=](auto &&source) {
                ros::Publisher publisher(rxros::node::get_handle().advertise<T>(topic, queue_size));
                source.subscribe_on(rxcpp::synchronize_new_thread()).subscribe(
                    [=](const T& msg) {publisher.publish(msg);});
                return source;};}


        auto send_transform(const std::string &frame_id, const std::string &child_frame_id) {
            return [=](auto &&source) {
                tf::TransformBroadcaster transformBroadcaster;
                source.subscribe_on(rxcpp::synchronize_new_thread()).subscribe(
                    [&](const tf::Transform& tf) {transformBroadcaster.sendTransform(tf::StampedTransform(tf, ros::Time::now(), frame_id, child_frame_id));});
                return source;};}


        template <class Observable>
        auto join_with_latest_from(const Observable& observable) {
            return [=](auto&& source) {
                return source.with_latest_from (
                    [=](const auto o1, const auto o2) {
                        return std::make_tuple(o1, o2);}, observable);};}

    } // end namespace operators
} // end namespace rxros


#endif //RXROS_H
