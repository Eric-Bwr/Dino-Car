#include "Arduino.h"
#include <filesystem>
#include <regex>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>

Arduino::Arduino() :
    isRunning(false) {}

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

            fd = open(chosenPort.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
            if (fd < 0) {
                throw std::runtime_error("Failed to open serial port: " + std::string(strerror(errno)));
            }

            struct termios tty;
            if (tcgetattr(fd, &tty) != 0) {
                throw std::runtime_error("Failed to get serial attributes: " + std::string(strerror(errno)));
            }

            cfsetospeed(&tty, B115200);
            cfsetispeed(&tty, B115200);

            // 8N1 (8 data bits, no parity, 1 stop bit)
            tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
            tty.c_cflag &= ~(PARENB | PARODD); // No parity
            tty.c_cflag &= ~CSTOPB;            // 1 stop bit
            tty.c_cflag &= ~CRTSCTS;           // No flow control
            tty.c_cflag |= CREAD | CLOCAL;     // Enable receiver, ignore modem control lines

            tty.c_lflag = 0;                   // No signaling chars, no echo, no canonical processing
            tty.c_oflag = 0;                   // No remapping, no delays
            tty.c_cc[VMIN] = 1;                // Read blocks until 1 byte arrives
            tty.c_cc[VTIME] = 0;               // No timeout

            if (tcsetattr(fd, TCSANOW, &tty) != 0) {
                throw std::runtime_error("Failed to set serial attributes: " + std::string(strerror(errno)));
            }
            processSerial();
        } catch (const std::exception& e) {
            std::cerr << "Arduino connection failed: " << e.what() << std::endl;
            isRunning = false;
        }
    });
}

void Arduino::stop() {
    isRunning = false;
}

void Arduino::setGearAngle(int angle) {
    gearAngle = angle;
}

VehicleData Arduino::getData() const {
    return data;
}

std::string Arduino::findArduinoPort() {
    const std::string path = "/dev/serial/by-id/";
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Path not found: " + path);
    }

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        std::string symlink = entry.path().string();
        if (symlink.find("Arduino") != std::string::npos || symlink.find("usbserial") != std::string::npos) {
            std::string realPath = std::filesystem::canonical(entry.path()).string();
            return realPath;
        }
    }

    throw std::runtime_error("No Arduino found");
}

void Arduino::processSerial() {
    std::string buffer;
    std::regex re(R"(G:(\d+),R:(\d+),T:([\d\.]+),Th:([\d\.]+),L:([\d\.]+),A:([\d\.]+),V:([\d\.]+))");
    char c;
    while (isRunning) {
        try {
            ssize_t n = read(fd, &c, 1);
            if (n == 1) {
                if (c == '\n') {
                    std::smatch match;
                    if (std::regex_search(buffer, match, re) && match.size() == 8) {
                        data.currentGear = std::stoi(match[1]);
                        data.engineRpm = std::stoi(match[2]);
                        data.coolantTemp = std::stof(match[3]);
                        data.throttle = std::stof(match[4]);
                        data.engineLoad = std::stof(match[5]);
                        data.ambientTemp = std::stof(match[6]);
                        data.voltage = std::stof(match[7]);
                    }
                    buffer.clear();
                } else {
                    buffer += c;
                }

                if (gearAngle != GEAR_NONE && gearAngle >= SHIFT_UP_ANGLE && gearAngle <= SHIFT_DOWN_ANGLE) {
                    std::string command = "G:" + std::to_string(gearAngle) + "\n";
                    write(fd, command.c_str(), command.size());
                    gearAngle = GEAR_NONE;
                }
            } else if (n < 0) {
                throw std::runtime_error("Serial read error: " + std::string(strerror(errno)));
            }
        } catch (const std::exception& e) {
            if (isRunning) std::cerr << "Serial error: " << e.what() << std::endl;
            break;
        }
    }
}
