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

int gearGoal = GEAR_N;

Uint32 lastShiftTime = 0;
bool servoDetached = false;
bool canShift = true;
const Uint32 SHIFT_COOLDOWN_MS = 1600;
const int BACKLASH_COMPENSATION = 10;
enum ShiftDirection { SHIFT_NONE, SHIFT_UP, SHIFT_DOWN };
ShiftDirection lastShiftDirection = SHIFT_NONE;

int getServoAngle(int fromGear, int toGear) {
    if (fromGear == GEAR_N && toGear == GEAR_1) return SHIFT_DOWN_ANGLE - 5;
    if (fromGear == GEAR_1 && toGear == GEAR_N) return SHIFT_UP_ANGLE + 10;
    if (toGear == fromGear) return NEUTRAL_ANGLE;
    if (toGear > fromGear) return SHIFT_UP_ANGLE;
    if (toGear < fromGear) return SHIFT_DOWN_ANGLE;
    return NEUTRAL_ANGLE;
}

void shiftUp() {
    if (canShift && gearGoal < GEAR_6) {
        gearGoal++;
        lastShiftTime = SDL_GetTicks();
        servoDetached = false;
        canShift = false;
        lastShiftDirection = SHIFT_UP;
    }
}

void shiftDown() {
    if (canShift && gearGoal > GEAR_N) {
        gearGoal--;
        lastShiftTime = SDL_GetTicks();
        servoDetached = false;
        canShift = false;
        lastShiftDirection = SHIFT_DOWN;
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
#if IS_RASPI
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

        Uint32 now = SDL_GetTicks();
        if (!canShift && (now - lastShiftTime > SHIFT_COOLDOWN_MS)) {
            canShift = true;
        }

        if (gearGoal == data.currentGear) {
            if (!servoDetached && (now - lastShiftTime > SHIFT_COOLDOWN_MS)) {
                arduino.setGearAngle(GEAR_NONE);
                servoDetached = true;
            } else if (!servoDetached) {
                int compensatedAngle = NEUTRAL_ANGLE;
                if (lastShiftDirection == SHIFT_UP) {
                    compensatedAngle = NEUTRAL_ANGLE + BACKLASH_COMPENSATION;
                } else if (lastShiftDirection == SHIFT_DOWN) {
                    compensatedAngle = NEUTRAL_ANGLE - BACKLASH_COMPENSATION;
                }
                arduino.setGearAngle(compensatedAngle);
            }
        } else {
            int angle = getServoAngle(data.currentGear, gearGoal);
            arduino.setGearAngle(angle);
            lastShiftTime = SDL_GetTicks();
            servoDetached = false;
        }

        data.gearGoal = data.currentGear == gearGoal ? GEAR_NONE : gearGoal;
        renderer.render(data, calculateSpeed(data.engineRpm, data.currentGear));
        SDL_Delay(16);
    }
}
