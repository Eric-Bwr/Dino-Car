#include <iostream>
#include <SDL.h>
#if IS_RASPI
#include <gpiod.h>
#endif
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

int gearGoal = -2;
bool clutchPressed = false;
Uint32 lastShiftTime = 0;
bool servoDetached = false;
bool canShift = true;
const Uint32 SHIFT_COOLDOWN_MS = 1400;
const int BACKLASH_COMPENSATION = 4;
enum ShiftDirection { SHIFT_NONE, SHIFT_UP, SHIFT_DOWN };
ShiftDirection lastShiftDirection = SHIFT_NONE;

int getServoAngle(int fromGear, int toGear) {
    if (fromGear == GEAR_N && toGear == GEAR_1) return SHIFT_DOWN_ANGLE;
    if (fromGear == GEAR_1 && toGear == GEAR_N) return SHIFT_UP_ANGLE;
    if (fromGear == GEAR_2 && toGear == GEAR_N) return SHIFT_DOWN_ANGLE - 13;
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

const unsigned int PROX_PIN = 14;  // GPIO14 (Pin 8)
const unsigned int BTN1_PIN = 12;  // GPIO12 (Pin 32)
const unsigned int BTN2_PIN = 16;  // GPIO16 (Pin 36)

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
    struct gpiod_chip *chip = gpiod_chip_open_by_name("gpiochip0");
    struct gpiod_line *lineProx = gpiod_chip_get_line(chip, PROX_PIN);
    struct gpiod_line *lineBtn1 = gpiod_chip_get_line(chip, BTN1_PIN);
    struct gpiod_line *lineBtn2 = gpiod_chip_get_line(chip, BTN2_PIN);

    gpiod_line_request_input_flags(lineProx, "cluster", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
    gpiod_line_request_input_flags(lineBtn1, "cluster", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
    gpiod_line_request_input_flags(lineBtn2, "cluster", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
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
            data.clutchPressed = clutchPressed = !data.clutchPressed;
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

        if (gearGoal == -2) {
            gearGoal = data.currentGear;
        }

        int proximity = gpiod_line_get_value(lineProx);
        int button1 = gpiod_line_get_value(lineBtn1);
        int button2 = gpiod_line_get_value(lineBtn2);

        data.clutchPressed = clutchPressed = proximity == 0;

        if(button1 == 0) {
            shiftUp();
        }

        if(button2 == 0) {
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

        float calculatedSpeed = calculateSpeed(data.engineRpm, data.currentGear);
        if (clutchPressed) {
            calculatedSpeed = -1.0f;
        }

        data.gearGoal = data.currentGear == gearGoal ? GEAR_NONE : gearGoal;
        renderer.render(data, calculatedSpeed);
        SDL_Delay(16);
    }
}
