#include <SDL2_gfxPrimitives.h>
#include "Renderer.h"

void Renderer::renderBackground(){
    SDL_Rect bgRect = {0, 0, width, height};
    SDL_RenderCopy(renderer, bgTexture, NULL, &bgRect);

    SDL_Color rpmBackColor = {50, 50, 50, 100};
    drawRPMArc(RPM_ARC_START_ANGLE, RPM_ARC_END_ANGLE, rpmBackColor, true);

    renderLoadThrottleBarBackground();
    renderTrackText();
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

void Renderer::renderTrackText(){
    SDL_Color outlineColor = {216, 67, 21, 255};
    SDL_Color fillColor = {0, 0, 0, 255};
    SDL_Surface* gearSurface = TTF_RenderText_Blended(trackFont, "TRACK", {255,255,255,255});
    SDL_Texture* gearTexture = SDL_CreateTextureFromSurface(renderer, gearSurface);
    SDL_Rect gearRect = {
            (width - gearSurface->w) / 2 + 90,
            centerY - gearSurface->h / 2 + 84,
            gearSurface->w,
            gearSurface->h
    };

    SDL_SetTextureColorMod(gearTexture, outlineColor.r, outlineColor.g, outlineColor.b);
    for (int dx = -1; dx <= 2; ++dx) {
        for (int dy = -1; dy <= 2; ++dy) {
            if (dx == 0 && dy == 0) continue;
            SDL_Rect outlineRect = gearRect;
            outlineRect.x += dx;
            outlineRect.y += dy;
            SDL_RenderCopy(renderer, gearTexture, nullptr, &outlineRect);
        }
    }

    SDL_SetTextureColorMod(gearTexture, fillColor.r, fillColor.g, fillColor.b);
    SDL_RenderCopy(renderer, gearTexture, nullptr, &gearRect);
    SDL_FreeSurface(gearSurface);
    SDL_DestroyTexture(gearTexture);
}