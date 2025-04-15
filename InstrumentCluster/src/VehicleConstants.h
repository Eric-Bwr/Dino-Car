#pragma once

#include <array>

const int RPM_MAX = 12000;

const float WHEEL_DIAMETER_MM = 360.4f;
const float FINAL_DRIVE_RATIO = 12.42f;

#define IS_RASPI (__arm__ || __aarch64__)

const std::array<float, 7> GEAR_RATIOS = {
    0.0f,   // Neutral
    3.909f, // 1st gear
    2.056f, // 2nd gear
    1.269f, // 3rd gear
    0.964f, // 4th gear
    0.780f, // 5th gear
    0.680f  // 6th gear
};

struct VehicleData {
    int currentGear = 0;
    int currentRpm = 0;
    float currentTemp = 0.0f;
    float currentThrottle = 0.0f;
    float currentLoad = 0.0f;
    float currentAmbient = 0.0f;
    float currentVoltage = 0.0f;
};