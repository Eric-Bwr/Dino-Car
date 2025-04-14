#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();
    void start();
    void render(int gear, int rpm, float temp);
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    double screenAngle;
    int width, height;
};
