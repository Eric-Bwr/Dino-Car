#include "Arduino.h"
#include "Renderer.h"
#include "VehicleConstants.h"
#include <array>
#include <cmath>

float calculateSpeed(int rpm, int gear) {
    if (gear <= 0 || gear >= GEAR_RATIOS.size() || rpm == 0) {
        return 0.0f;
    }
    float wheelCircumference = M_PI * WHEEL_DIAMETER_MM;
    float speedMmPerMinute = rpm * wheelCircumference / (GEAR_RATIOS[gear] * FINAL_DRIVE_RATIO);
    return speedMmPerMinute * 0.000001f * 60.0f;
}

int main() {
    Arduino arduino;
    arduino.start();

    Renderer renderer(800, 480);
    renderer.start();

    VehicleData data;
    while (true) {
#if not IS_RASPI
        data.currentRpm += 100;
        if (data.currentRpm > RPM_MAX) {
            data.currentGear++;
            if (data.currentGear == 7) {
                data.currentGear = 0;
            }
            data.currentRpm = 0;
        }
        data.currentAmbient = 20.5;
        data.currentVoltage = 11.9;
        data.currentCoolantTemp = 14.7;
        data.currentLoad = ((float)data.currentRpm / RPM_MAX) * 100.0f;
        data.currentThrottle = ((float)data.currentRpm / RPM_MAX) * 80.0f;
#else
        data = arduino.getData();
#endif
        renderer.render(data, calculateSpeed(data.currentRpm, data.currentGear));
        SDL_Delay(16);
    }
}
