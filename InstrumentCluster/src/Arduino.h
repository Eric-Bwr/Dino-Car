#pragma once

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
    void setGearAngle(int angle);
    VehicleData getData() const;
private:
    void processSerial();
    std::string findArduinoPort();
    std::atomic<bool> isRunning;
    VehicleData data;
    std::thread serialThread;
    std::atomic_int gearAngle = -1;
    int fd;
};
