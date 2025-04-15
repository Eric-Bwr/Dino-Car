#include "Arduino.h"
#include <boost/filesystem.hpp>
#include <regex>
#include <iostream>

Arduino::Arduino() :
    serialPort(ioContext),
    isRunning(false),
    currentGear(0),
    currentRpm(0),
    currentTemp(0.0f) {}

Arduino::~Arduino() {
    stop();
    if (serialThread.joinable()){
        serialThread.join();
    }
}

void Arduino::start() {
    if (isRunning) return;

    isRunning = true;
    serialThread = std::thread([this]
    {
        try {
            std::string chosenPort = findArduinoPort();
            std::cout << "Arduino on: " << chosenPort << std::endl;
            serialPort.open(chosenPort);
            serialPort.set_option(boost::asio::serial_port_base::baud_rate(9600));
            readSerial();
        } catch (const std::exception& e) {
            std::cerr << "Arduino connection failed: " << e.what() << std::endl;
            isRunning = false;
        }
    });
}

void Arduino::stop() {
    isRunning = false;
    if (serialPort.is_open()) serialPort.close();
}

void Arduino::getData(int& gear, int& rpm, float& temp) {
    std::lock_guard lock(dataMutex);
    gear = currentGear.load();
    rpm = currentRpm.load();
    temp = currentTemp.load();
}

std::string Arduino::findArduinoPort() {
    const std::string path = "/dev/serial/by-id/";
    if (!boost::filesystem::exists(path)) {
        throw std::runtime_error("Path not found: " + path);
    }

    for (const auto& entry : boost::filesystem::directory_iterator(path)) {
        std::string symlink = entry.path().string();
        if (symlink.find("Arduino") != std::string::npos || symlink.find("usbserial") != std::string::npos) {
            std::string realPath = boost::filesystem::canonical(symlink).string();
            return realPath;
        }
    }

    throw std::runtime_error("No Arduino found");
}

void Arduino::readSerial() {
    // "G:0,R:0,T:19.70,Th:10.98,L:0.00,A:21.00"
    std::string buffer;
    char c;
    while (isRunning) {
        try {
            boost::asio::read(serialPort, boost::asio::buffer(&c, 1));
            if (c == '\n') {
                if (buffer.find("G:") != std::string::npos &&
                    buffer.find(",R:") != std::string::npos &&
                    buffer.find(",T:") != std::string::npos) {

                    std::lock_guard lock(dataMutex);
                    size_t gearPos = buffer.find("G:");
                    size_t rpmPos = buffer.find(",R:");
                    size_t tempPos = buffer.find(",T:");

                    currentGear = std::stoi(buffer.substr(gearPos + 2, rpmPos - (gearPos + 2)));
                    currentRpm = std::stoi(buffer.substr(rpmPos + 3, tempPos - (rpmPos + 3)));
                    currentTemp = std::stof(buffer.substr(tempPos + 3));
                }
                buffer.clear();
            } else {
                buffer += c;
            }
        } catch (const std::exception& e) {
            if (isRunning) std::cerr << "Serial error: " << e.what() << std::endl;
            break;
        }
    }
}
