#pragma once

#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include "VehicleConstants.h"

const float RPM_ARC_START_ANGLE = 90.0f + 30.0f;
const float RPM_ARC_END_ANGLE = 90.0f + 330.0f;
const float THROTTLE_ANGLE_START = 155.0f;
const float THROTTLE_ANGLE_END = 200.0f;
const float LOAD_ANGLE_START = -20.0f;
const float LOAD_ANGLE_END = 25.0f;

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();
    void start();
    void render(const VehicleData& data, float speed);
private:
    void renderGear(int gear, bool goal = false);
    void renderSpeed(float speed);
    void renderRPM();
    void drawNeedle(float rpmRatio);
    void drawRPMArc(float startAngle, float endAngle, SDL_Color color, bool ticks);
    void drawRPMNumbers();
    void renderLoadThrottleBars();
    void renderInfoTexts(float ambientTemp, float coolantTemp, float batteryVoltage, bool clutchPressed);
    void renderTrackText();
    void generateArcPoints(float startAngle, float endAngle, int outerRad, int innerRad, std::vector<Sint16>& vX, std::vector<Sint16>& vY, bool outline = false) const;
    void preRenderBackground();
    void renderLoadThrottleBarBackground();
    void renderLoadThrottleBar(float startAngle, float endAngle, SDL_Color color, bool outline);
    SDL_Texture* loadTexture(const std::string& filePath);
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* gearFont;
    TTF_Font* gearGoalFont;
    TTF_Font* speedFont;
    TTF_Font* numberFont;
    TTF_Font* trackFont;
    TTF_Font* infoFont;
    SDL_Texture* bgTexture;
    SDL_Texture* tempTexture;
    SDL_Texture* coolantTexture;
    SDL_Texture* engineLoadTexture;
    SDL_Texture* batteryTexture;
    SDL_Texture* throttleTexture;
    SDL_Texture* clutchTexture;
    SDL_Texture* absTexture;
    SDL_Texture* tcTexture;
    SDL_Texture* renderedBackgroundTexture;
    SDL_Texture* renderTexture;
    SDL_Rect bgRect;
    int width, height;
    int centerX, centerY;
    int radius, innerRadius;
    float smoothedRpm = 0.0f;
    float smoothedLoad = 0.0f;
    float smoothedThrottle = 0.0f;
    const float smoothingFactor = 0.7f;
};
