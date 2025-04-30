#include "Renderer.h"
#include <SDL2_gfxPrimitives.h>
#include <iostream>

void Renderer::preRenderBackground(){
    renderedBackgroundTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
    if (!renderedBackgroundTexture) {
        std::cerr << "Failed to create background texture: " << SDL_GetError() << std::endl;
        return;
    }
    SDL_SetRenderTarget(renderer, renderedBackgroundTexture);

    SDL_Rect bgRect = {0, 0, width, height};
    SDL_RenderCopy(renderer, bgTexture, NULL, &bgRect);

    SDL_Color rpmBackColor = {50, 50, 50, 100};
    drawRPMArc(RPM_ARC_START_ANGLE, RPM_ARC_END_ANGLE, rpmBackColor, true);

    renderLoadThrottleBarBackground();

    SDL_SetRenderTarget(renderer, NULL);
}

void Renderer::renderLoadThrottleBarBackground(){
    int customRadius = radius + 120;
    auto renderTicks = [&](float startAngle, float endAngle, int tickRadius) {
        int numTicks = 6;
        int tickLength = 20;
        int tickThickness = 3;
        float angleRange = startAngle - endAngle;
        for (int i = 0; i <= numTicks; ++i) {
            if (i == 0 || i == numTicks){
                continue;
            }
            int customTickLength = tickLength;
            int customTickThickness = tickThickness;
            if (i % 2 == 0) {
                customTickLength = 10;
                customTickThickness = 3;
            }
            float tickAngle = startAngle - ((float)i / (float)numTicks) * angleRange;
            float tickAngleRad = tickAngle * M_PI / 180.0f;
            int tickOuterX = centerX - tickRadius * cosf(tickAngleRad);
            int tickOuterY = centerY + tickRadius * sinf(tickAngleRad);
            int tickInnerX = centerX - (tickRadius - customTickLength) * cosf(tickAngleRad);
            int tickInnerY = centerY + (tickRadius - customTickLength) * sinf(tickAngleRad);
            thickLineRGBA(renderer, tickOuterX, tickOuterY, tickInnerX, tickInnerY, customTickThickness, 170, 170, 170, 200);
        }
    };

    renderTicks(LOAD_ANGLE_START, LOAD_ANGLE_END, customRadius);
    renderTicks(THROTTLE_ANGLE_START, THROTTLE_ANGLE_END, customRadius);

    renderLoadThrottleBar(LOAD_ANGLE_START, LOAD_ANGLE_END, {255, 255, 255, 200}, true);
    renderLoadThrottleBar(THROTTLE_ANGLE_START, THROTTLE_ANGLE_END, {255, 255, 255, 200}, true);
}