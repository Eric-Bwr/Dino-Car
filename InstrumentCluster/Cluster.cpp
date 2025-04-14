#include "Arduino.h"
#include "Renderer.h"

int main() {
    int gear = 0;
    int rpm = 0;
    float temp = 0.0f;

    Arduino arduino;
    arduino.start();

    Renderer renderer(800, 480);
    renderer.start();

    while (true) {
        arduino.getData(gear, rpm, temp);
        renderer.render(gear, rpm, temp);
        SDL_Delay(16);
    }
}
