#include "Arduino.h"
#include <boost/filesystem.hpp>
#include <regex>
#include <iostream>

Arduino::Arduino() :
    isRunning(false),
    serialPort(ioContext) {}

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
            serialPort.set_option(boost::asio::serial_port_base::baud_rate(115200));
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

VehicleData Arduino::getData() const {
    return data;
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
    std::string buffer;
    std::regex re(R"(G:(\d+),R:(\d+),T:([\d\.]+),Th:([\d\.]+),L:([\d\.]+),A:([\d\.]+),V:([\d\.]+))");
    char c;
    while (isRunning) {
        try {
            boost::asio::read(serialPort, boost::asio::buffer(&c, 1));
            if (c == '\n') {
                std::smatch match;
                if (std::regex_search(buffer, match, re) && match.size() == 8) {
                    data.currentGear = std::stoi(match[1]);
                    data.currentRpm = std::stoi(match[2]);
                    data.currentCoolantTemp = std::stof(match[3]);
                    data.currentThrottle = std::stof(match[4]);
                    data.currentLoad = std::stof(match[5]);
                    data.currentAmbient = std::stof(match[6]);
                    data.currentVoltage = std::stof(match[7]);
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
