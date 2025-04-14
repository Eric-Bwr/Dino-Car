#include "Renderer.h"
#include <iostream>

#define IS_RASPI (__arm__ || __aarch64__)

Renderer::Renderer(int width, int height) : window(nullptr), renderer(nullptr), font(nullptr), width(width), height(height){
    screenAngle =
#if IS_RASPI
        180.0;
#else
        0.0;
#endif
}

Renderer::~Renderer(){
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Renderer::start(){
#if IS_RASPI
    setenv("SDL_VIDEODRIVER", "kmsdrm", 1);
#endif

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL Init Failed: " << SDL_GetError() << std::endl;
        return;
    }

    TTF_Init();

    window = SDL_CreateWindow("Cluster",
#if IS_RASPI
        0, 0,
#else
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
#endif
        width, height,
#if IS_RASPI
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS
#else
        SDL_WINDOW_SHOWN
#endif
    );

#if IS_RASPI
    SDL_ShowCursor(SDL_DISABLE);
#endif

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 50);
}

void Renderer::render(){
    for (int y = 0; y < height; y++) {
        float ratio = (float)y / height;
        SDL_SetRenderDrawColor(renderer,
            0, 0,
            static_cast<Uint8>(255 * (1.0 - ratio)),
            255);
        SDL_RenderDrawLine(renderer, 0, y, width, y);
    }

    int gear = 0;
    int rpm = 2;
    static float testTemp = 0;
    std::string texts[] = {
        "Gear: " + std::to_string(gear),
        "RPM: " + std::to_string(rpm),
        "Temp: " + std::to_string(testTemp) + "°C"
    };
    testTemp += 0.0002f;

    const SDL_Color textColor = {255, 255, 255, 255};
    int yOffset = -50;
    for (const auto& text : texts) {
        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), textColor);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {
            (width - surface->w)/2,
            (height - surface->h)/2 + yOffset,
            surface->w, surface->h
        };
        SDL_RenderCopyEx(renderer, texture, nullptr, &rect, screenAngle, nullptr, SDL_FLIP_NONE);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
        yOffset += 50;
    }

    SDL_RenderPresent(renderer);
}
