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
    int gear = 0;
    int rpm = 0;
    float temp = 0.0f;
    float speed = 0.0f;

    Arduino arduino;
    arduino.start();

    Renderer renderer(800, 480);
    renderer.start();

    while (true) {
        if (false) {
            rpm += 100;
            if (rpm > RPM_MAX) {
                gear++;
                if (gear == 7) {
                    gear = 0;
                }
                rpm = 0;
            }
        }else {
            arduino.getData(gear, rpm, temp);
        }
        speed = calculateSpeed(rpm, gear);
        renderer.render(gear, rpm, temp, speed);
        // SDL_Delay(16);
    }
}
