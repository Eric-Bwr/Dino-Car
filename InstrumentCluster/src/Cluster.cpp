#include <SDL.h>
#include "Arduino.h"
#include "Renderer.h"
#include "VehicleConstants.h"

float calculateSpeed(int rpm, int gear) {
    if (gear <= 0 || gear >= GEAR_RATIOS.size() || rpm == 0) {
        return 0.0f;
    }
    float wheelCircumference = M_PI * WHEEL_DIAMETER_MM;
    float speedMmPerMinute = rpm * wheelCircumference / (GEAR_RATIOS[gear] * FINAL_DRIVE_RATIO);
    return speedMmPerMinute * 0.000001f * 60.0f;
}

enum Gear {
    GEAR_N = 0,
    GEAR_1 = 1,
    GEAR_2 = 2,
    GEAR_3 = 3,
    GEAR_4 = 4,
    GEAR_5 = 5,
    GEAR_6 = 6
};

const int NEUTRAL_ANGLE = 88;
const int SHIFT_UP_ANGLE = NEUTRAL_ANGLE - 20;
const int SHIFT_DOWN_ANGLE = NEUTRAL_ANGLE + 20;

int currentGear = GEAR_N;
int gearGoal = GEAR_N;

Uint32 lastShiftTime = 0;
bool servoDetached = false;

int getServoAngle(int fromGear, int toGear) {
    if (toGear == fromGear) return NEUTRAL_ANGLE;
    if (toGear > fromGear) return SHIFT_UP_ANGLE;
    if (toGear < fromGear) return SHIFT_DOWN_ANGLE;
    return NEUTRAL_ANGLE;
}

void shiftUp() {
    if (gearGoal < GEAR_6) {
        gearGoal++;
        lastShiftTime = SDL_GetTicks();
        servoDetached = false;
    }
}

void shiftDown() {
    if (gearGoal > GEAR_N) {
        gearGoal--;
        lastShiftTime = SDL_GetTicks();
        servoDetached = false;
    }
}

int main() {
    Arduino arduino;
    arduino.start();

    Renderer renderer(800, 480);
    renderer.start();

    SDL_Event event;
    bool running = true;

    VehicleData data;

    lastShiftTime = SDL_GetTicks();

    while (running) {
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
        data.coolantTemp = ((float)data.engineRpm / RPM_MAX) * THROTTLE_MAX;
        data.engineLoad = ((float)data.engineRpm / RPM_MAX) * 100.0f;
        data.throttle = ((float)data.engineRpm / RPM_MAX) * 72.0f;
#else
        data = arduino.getData();
#endif
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_a:
                        shiftDown();
                        break;
                    case SDLK_d:
                        shiftUp();
                        break;
                }
            }
        }

        if (gearGoal == currentGear) {
            Uint32 now = SDL_GetTicks();
            if (!servoDetached && (now - lastShiftTime > 3000)) {
                arduino.setGearAngle(-1);
                servoDetached = true;
            } else if (!servoDetached) {
                arduino.setGearAngle(NEUTRAL_ANGLE);
            }
        } else {
            int angle = getServoAngle(currentGear, gearGoal);
            arduino.setGearAngle(angle);
            lastShiftTime = SDL_GetTicks();
            servoDetached = false;
#if not IS_RASPI
            currentGear = gearGoal;
#endif
        }

        data.currentGear = currentGear;
        data.gearGoal = currentGear == gearGoal ? -1 : gearGoal;
        renderer.render(data, calculateSpeed(data.engineRpm, data.currentGear));
        SDL_Delay(16);
    }
}
