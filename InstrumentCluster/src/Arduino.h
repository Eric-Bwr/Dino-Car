#pragma once

#include <boost/asio.hpp>
#include <string>
#include <atomic>
#include <thread>
#include "VehicleConstants.h"

class Arduino {
public:
    Arduino();
    ~Arduino();
    void start();
    void stop();
    VehicleData getData() const;
private:
    void readSerial();
    std::string findArduinoPort();
    std::atomic<bool> isRunning;
    VehicleData data;
    boost::asio::io_context ioContext;
    boost::asio::serial_port serialPort;
    std::thread serialThread;
};
