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
        data.engineRpm += 100;
        if (data.engineRpm > RPM_MAX) {
            data.currentGear++;
            if (data.currentGear == 7) {
                data.currentGear = 0;
            }
            data.engineRpm = 0;
            data.clutchPressed = !data.clutchPressed;
        }
        data.ambientTemp = 20.5;
        data.voltage = ((float)data.engineRpm / RPM_MAX) * 15.0f;
        data.coolantTemp = ((float)data.engineRpm / RPM_MAX) * 80.0f;
        data.engineLoad = ((float)data.engineRpm / RPM_MAX) * 100.0f;
        data.throttle = ((float)data.engineRpm / RPM_MAX) * 80.0f;
#else
        data = arduino.getData();
#endif
        renderer.render(data, calculateSpeed(data.engineRpm, data.currentGear));
        SDL_Delay(16);
    }
}
