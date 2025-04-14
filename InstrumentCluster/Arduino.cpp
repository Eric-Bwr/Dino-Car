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

void Arduino::start(const std::string& port) {
    if (isRunning) return;

    try {
        serialPort.open(port.empty() ? findArduinoPort() : port);
        serialPort.set_option(boost::asio::serial_port_base::baud_rate(9600));
        isRunning = true;
        serialThread = std::thread(&Arduino::readSerial, this);
    } catch (const std::exception& e) {
        std::cerr << "Arduino connection failed: " << e.what() << std::endl;
    }
}

void Arduino::stop() {
    isRunning = false;
    if (serialPort.is_open()) serialPort.close();
}

void Arduino::getData(int& gear, int& rpm, float& temp) {
    std::lock_guard<std::mutex> lock(dataMutex);
    gear = currentGear.load();
    rpm = currentRpm.load();
    temp = currentTemp.load();
}

bool Arduino::isConnected() const {
    return serialPort.is_open() && isRunning;
}

std::string Arduino::findArduinoPort() {
    for (const auto& entry : boost::filesystem::directory_iterator("/dev")) {
        std::string port = entry.path().string();
        if (std::regex_match(port, std::regex(".*(ttyUSB|ttyACM|ttyAMA|ttyS)\\d+"))) {
            try {
                boost::asio::serial_port testPort(ioContext);
                testPort.open(port);
                testPort.set_option(boost::asio::serial_port_base::baud_rate(9600));

                std::string response;
                char c;
                while (boost::asio::read(testPort, boost::asio::buffer(&c, 1))) {
                    if (c == '\n') break;
                    response += c;
                }

                if (response.find("Arduino") != std::string::npos) {
                    return port;
                }
            } catch (...) {}
        }
    }
    throw std::runtime_error("No Arduino found");
}

void Arduino::readSerial() {
    std::string buffer;
    char c;
    while (isRunning) {
        try {
            boost::asio::read(serialPort, boost::asio::buffer(&c, 1));
            if (c == '\n') {
                if (buffer.find("G:") != std::string::npos &&
                    buffer.find(",R:") != std::string::npos &&
                    buffer.find(",T:") != std::string::npos) {

                    std::lock_guard<std::mutex> lock(dataMutex);
                    size_t gearPos = buffer.find("G:");
                    size_t rpmPos = buffer.find(",R:");
                    size_t tempPos = buffer.find(",T:");

                    currentGear = std::stoi(buffer.substr(gearPos+2, rpmPos - (gearPos+2)));
                    currentRpm = std::stoi(buffer.substr(rpmPos+3, tempPos - (rpmPos+3)));
                    currentTemp = std::stof(buffer.substr(tempPos+3));
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
