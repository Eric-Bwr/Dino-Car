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

    SDL_Event event;
    bool running = true;

    int gearGoal = -1;

    VehicleData data;
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
        data.throttle = ((float)data.engineRpm / RPM_MAX) * 80.0f;
#else
        data = arduino.getData();
#endif
        while (SDL_PollEvent(&event)) {
            arduino.setGearAngle(-1);
            if (event.type == SDL_QUIT) {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_a:
                        gearGoal--;
                        if(gearGoal < -1){
                            gearGoal = -1;
                        }
                        break;
                    case SDLK_d:
                        gearGoal++;
                        if(gearGoal > 6){
                            gearGoal = 6;
                        }
                        break;
                    case SDLK_s:
                        arduino.setGearAngle(0);
                        break;
                    case SDLK_x:
                        arduino.setGearAngle(90);
                        break;
                    case SDLK_w:
                        arduino.setGearAngle(180);
                        break;
                }
            }
        }
        data.gearGoal = gearGoal;
        renderer.render(data, calculateSpeed(data.engineRpm, data.currentGear));
        SDL_Delay(16);
    }
}
