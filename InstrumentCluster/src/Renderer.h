#pragma once

#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "VehicleConstants.h"

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();
    void start();
    void render(const VehicleData& data, float speed);
private:
    void renderGear(int gear);
    void renderSpeed(float speed);
    void renderRPM(int rpm);
    void drawNeedle(float rpmRatio);
    void drawRPMArc(float startAngle, float endAngle, SDL_Color color);
    void drawRPMNumbers();
    void renderTemperatureGauge(float temperature, const std::string& label, int x, int y);
    void renderLoadThrottleBars(float load, float throttle);
    SDL_Texture* loadTexture(const std::string& filePath);
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* gearFont;
    TTF_Font* speedFont;
    TTF_Font* numberFont;
    SDL_Texture* tempTexture;
    SDL_Texture* coolantTexture;
    SDL_Texture* engineLoadTexture;
    SDL_Texture* batteryTexture;
    SDL_Texture* throttleTexture;
    double screenAngle;
    int width, height;
    int centerX, centerY;
    int radius, innerRadius;
};
