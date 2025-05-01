#pragma once

#include <vector>

const int RPM_MAX = 12000;
const float THROTTLE_MAX = 80.0f;
const int WARNING_LIGHTS_RPM = 8000;
const int WARNING_ARC_RPM = 8500;

const float WHEEL_DIAMETER_MM = 420.0f;
const float PRIMARY_REDUCTION = 72.0f / 22.0f; // ~3.2727
const float SECONDARY_REDUCTION = 37.0f / 15.0f; // ~2.47
const float INVERT_GEARBOX_REDUCTION = 5.0f / 4.0f; // 1.25
const float FINAL_DRIVE_RATIO = PRIMARY_REDUCTION * SECONDARY_REDUCTION * INVERT_GEARBOX_REDUCTION;

const int NEUTRAL_ANGLE = 84;
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

const std::vector GEAR_RATIOS = {
    34.0f/12.0f,
    31.0f/15.0f,
    28.0f/18.0f,
    26.0f/21.0f,
    23.0f/22.0f,
    22.0f/24.0f
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