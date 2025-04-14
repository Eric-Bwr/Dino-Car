#include "Arduino.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>

const int screenWidth = 800;
const int screenHeight = 480;
const SDL_Color backgroundColor = {43, 43, 43, 255};
const SDL_Color textColor = {255, 255, 255, 255};
static float testTemp = 0.0f;
void render(SDL_Renderer* renderer, TTF_Font* font, Arduino& arduino) {
    for (int y = 0; y < screenHeight; y++) {
        float ratio = (float)y / screenHeight;
        SDL_SetRenderDrawColor(renderer,
            0, 0,
            static_cast<Uint8>(255 * (1.0 - ratio)),
            255);
        SDL_RenderDrawLine(renderer, 0, y, screenWidth, y);
    }

    int gear, rpm;
    float temp;
    arduino.getData(gear, rpm, temp);

    std::string texts[] = {
        "Gear: " + std::to_string(gear),
        "RPM: " + std::to_string(rpm),
        "Temp: " + std::to_string(testTemp) + "°C"
    };

    testTemp += 0.0002f;

    int yOffset = -50;
    for (const auto& text : texts) {
        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), textColor);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {
            (screenWidth - surface->w)/2,
            (screenHeight - surface->h)/2 + yOffset,
            surface->w, surface->h
        };
        SDL_RenderCopyEx(renderer, texture, nullptr, &rect, 180, nullptr, SDL_FLIP_NONE);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
        yOffset += 50;
    }

    SDL_RenderPresent(renderer);
}

int main() {
    setenv("SDL_VIDEODRIVER", "kmsdrm", 1);
    SDL_Init(SDL_INIT_VIDEO);
    SDL_ShowCursor(SDL_DISABLE);
    TTF_Init();
    SDL_Window* window = SDL_CreateWindow("Dashboard", 0, 0, screenWidth, screenHeight, SDL_WINDOW_FULLSCREEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 50);

    Arduino arduino;
    arduino.start("");

    if (!arduino.isConnected()) {
        std::cerr << "Failed to connect to Arduino" << std::endl;
    }

    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                goto cleanup;
            }
        }
        render(renderer, font, arduino);
        SDL_Delay(50);
    }

cleanup:
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
