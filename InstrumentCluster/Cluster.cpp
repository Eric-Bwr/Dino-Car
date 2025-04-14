#include <iostream>

#include "Arduino.h"
#include "Renderer.h"

int main() {
    Arduino arduino;
    arduino.start();

    Renderer renderer(800, 480);
    renderer.start();

    SDL_Event e;
    bool running = true;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
        }
        renderer.render();
        SDL_Delay(16);
    }

}
