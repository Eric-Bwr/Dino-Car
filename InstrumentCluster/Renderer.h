#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();
    void start();
    void render(int gear, int rpm, float temp, float speed);
private:
    void renderGear(int gear);
    void renderSpeed(float speed);
    void renderRPM(int rpm);
    void drawNeedle(float rpmRatio);
    void drawRPMArc(float startAngle, float endAngle, SDL_Color color);
    void drawRPMNumbers();
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* gearFont;
    TTF_Font* speedFont;
    TTF_Font* numberFont;
    double screenAngle;
    int width, height;
    int centerX, centerY;
    int radius, innerRadius;
};
