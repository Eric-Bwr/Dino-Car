#include <iostream>
#include <SDL.h>
#include <wiringPi.h>
#include "Arduino.h"
#include "Renderer.h"
#include "VehicleConstants.h"

float calculateSpeed(int rpm, int gear) {
    if (gear <= 0 || gear > GEAR_RATIOS.size() || rpm == 0) {
        return 0.0f;
    }
    float totalRatio = GEAR_RATIOS[gear-1] * FINAL_DRIVE_RATIO;
    float wheelCircumference = M_PI * WHEEL_DIAMETER_MM;
    return (rpm * wheelCircumference * 60.0f) / (totalRatio * 1000000.0f);
}

int gearGoal = GEAR_N;
bool clutchPressed = false;
Uint32 lastShiftTime = 0;
bool servoDetached = false;
bool canShift = true;
const Uint32 SHIFT_COOLDOWN_MS = 1400;
const int BACKLASH_COMPENSATION = 8;
enum ShiftDirection { SHIFT_NONE, SHIFT_UP, SHIFT_DOWN };
ShiftDirection lastShiftDirection = SHIFT_NONE;

int getServoAngle(int fromGear, int toGear) {
    if (fromGear == GEAR_N && toGear == GEAR_1) return SHIFT_DOWN_ANGLE;
    if (fromGear == GEAR_1 && toGear == GEAR_N) return SHIFT_UP_ANGLE;
    if (fromGear == GEAR_2 && toGear == GEAR_N) return SHIFT_DOWN_ANGLE - 10;
    if (toGear == fromGear) return NEUTRAL_ANGLE;
    if (toGear > fromGear) return SHIFT_UP_ANGLE;
    if (toGear < fromGear) return SHIFT_DOWN_ANGLE;
    return NEUTRAL_ANGLE;
}

void shiftUp() {
    if (canShift && gearGoal < GEAR_6 && clutchPressed) {
        gearGoal++;
        lastShiftTime = SDL_GetTicks();
        servoDetached = false;
        canShift = false;
        if (gearGoal == GEAR_1) {
            lastShiftDirection = SHIFT_DOWN;
            return;
        }
        lastShiftDirection = SHIFT_UP;
    }
}

void shiftDown() {
    if (canShift && gearGoal > GEAR_N && clutchPressed) {
        gearGoal--;
        lastShiftTime = SDL_GetTicks();
        servoDetached = false;
        canShift = false;
        lastShiftDirection = SHIFT_DOWN;
    }
}

const int PROX_PIN = 15;  // Proximity-Sensor (Pin 8)
const int BTN1_PIN = 26;  // Button 1 (Pin 32)
const int BTN2_PIN = 27;  // Button 2 (Pin 36)

int main() {
    Arduino arduino;
    arduino.start();

    Renderer renderer(800, 480);
    renderer.start();

    SDL_Event event;
    bool running = true;

    VehicleData data;

    lastShiftTime = SDL_GetTicks();
#if IS_RASPI
    wiringPiSetup();

    pinMode(PROX_PIN, INPUT);
    pinMode(BTN1_PIN, INPUT);
    pinMode(BTN2_PIN, INPUT);

    pullUpDnControl(PROX_PIN, PUD_UP);
    pullUpDnControl(BTN1_PIN, PUD_UP);
    pullUpDnControl(BTN2_PIN, PUD_UP);
#endif
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
#else
        data = arduino.getData();

        int proximity = digitalRead(PROX_PIN);
        int button1 = digitalRead(BTN1_PIN);
        int button2 = digitalRead(BTN2_PIN);

        data.clutchPressed = clutchPressed = proximity == LOW;

        if(button1 == LOW) {
            shiftUp();
        }

        if(button2 == LOW) {
            shiftDown();
        }
#endif

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
