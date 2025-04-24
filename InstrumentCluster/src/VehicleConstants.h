#pragma once

#include <array>

const int RPM_MAX = 12000;
const float THROTTLE_MAX = 72.0f;

const float WHEEL_DIAMETER_MM = 360.4f;
const float FINAL_DRIVE_RATIO = 12.42f;

const int NEUTRAL_ANGLE = 86;
const int SHIFT_UP_ANGLE = NEUTRAL_ANGLE - 25;
const int SHIFT_DOWN_ANGLE = NEUTRAL_ANGLE + 25;

#define IS_RASPI (__arm__ || __aarch64__)

enum Gear {
    GEAR_NONE = -1,
    GEAR_N = 0,
    GEAR_1 = 1,
    GEAR_2 = 2,
    GEAR_3 = 3,
    GEAR_4 = 4,
    GEAR_5 = 5,
    GEAR_6 = 6
};

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
    int gearGoal = GEAR_NONE;
    int currentGear = 0;
    int engineRpm = 0;
    float coolantTemp = 0.0f;
    float throttle = 0.0f;
    float engineLoad = 0.0f;
    float ambientTemp = 0.0f;
    float voltage = 0.0f;
    bool clutchPressed = false;
};