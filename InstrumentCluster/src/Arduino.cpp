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
            processSerial();
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

void Arduino::setGearAngle(int angle) {
    gearAngle = angle;
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

void Arduino::processSerial() {
    std::string buffer;
    std::vector<std::string> parts;
    char c;
    while (isRunning) {
        try {
            boost::asio::read(serialPort, boost::asio::buffer(&c, 1));
            if (c == '\n') {
                std::istringstream iss(buffer);
                std::string token;

                while (std::getline(iss, token, ',')) {
                    parts.push_back(token);
                }

                if (parts.size() >= 7) {
                    try {
                        data.currentGear = std::stoi(parts[0].substr(2));
                        data.engineRpm = std::stoi(parts[1].substr(2));
                        data.coolantTemp = std::stof(parts[2].substr(2));
                        data.throttle = std::stof(parts[3].substr(3));
                        data.engineLoad = std::stof(parts[4].substr(2));
                        data.ambientTemp = std::stof(parts[5].substr(2));
                        data.voltage = std::stof(parts[6].substr(2));
                    } catch (...) {
                        std::cerr << "Parsing error" << std::endl;
                    }
                }

                buffer.clear();
                parts.clear();
            } else {
                buffer += c;
            }
            if (gearAngle != GEAR_NONE && gearAngle >= SHIFT_UP_ANGLE && gearAngle <= SHIFT_DOWN_ANGLE) {
                std::string command = "G:" + std::to_string(gearAngle) + "\n";
                boost::asio::write(serialPort, boost::asio::buffer(command));
                gearAngle = GEAR_NONE;
            }
        } catch (const std::exception& e) {
            if (isRunning) std::cerr << "Serial error: " << e.what() << std::endl;
            break;
        }
    }
}
