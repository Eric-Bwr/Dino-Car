#pragma once

#include <boost/asio.hpp>
#include <string>
#include <atomic>
#include <mutex>
#include <thread>

class Arduino {
public:
    Arduino();
    ~Arduino();
    void start(const std::string& port);
    void stop();
    void getData(int& gear, int& rpm, float& temp);
    bool isConnected() const;

private:
    void readSerial();
    std::string findArduinoPort();

    boost::asio::io_context ioContext;
    boost::asio::serial_port serialPort;
    std::thread serialThread;
    std::atomic<bool> isRunning;
    std::mutex dataMutex;
    std::atomic<int> currentGear;
    std::atomic<int> currentRpm;
    std::atomic<float> currentTemp;
};
