#pragma once

#include <array>

const int RPM_MAX = 12000;
const float THROTTLE_MAX = 80.0f;

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
    int gearGoal = -1;
    int currentGear = 0;
    int engineRpm = 0;
    float coolantTemp = 0.0f;
    float throttle = 0.0f;
    float engineLoad = 0.0f;
    float ambientTemp = 0.0f;
    float voltage = 0.0f;
    bool clutchPressed = false;
};