#include "Renderer.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include <vector>

Renderer::Renderer(int width, int height) : window(nullptr), renderer(nullptr), width(width), height(height){
    centerX = width / 2;
    centerY = height / 2;
    radius = std::min(width, height) / 1.96;
    innerRadius = radius - 80;
    centerY += 10;
#if IS_RASPI
        screenAngle = 180.0;
#else
        screenAngle = 0.0;
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
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    renderTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
    if (!renderTexture) {
        std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
        return;
    }
    gearFont = TTF_OpenFont("../assets/trans.ttf", 270);
    gearGoalFont = TTF_OpenFont("../assets/trans.ttf", 80);
    speedFont = TTF_OpenFont("../assets/bebas.ttf", 100);
    numberFont = TTF_OpenFont("../assets/trans.ttf", 58);
    trackFont = TTF_OpenFont("../assets/trans.ttf", 26);
    infoFont = TTF_OpenFont("../assets/bebas.ttf", 50);
    bgTexture = loadTexture("../assets/bg.png");
    tempTexture = loadTexture("../assets/temp.png");
    coolantTexture = loadTexture("../assets/coolant.png");
    engineLoadTexture = loadTexture("../assets/load.png");
    batteryTexture = loadTexture("../assets/battery.png");
    throttleTexture = loadTexture("../assets/throttle.png");
    clutchTexture = loadTexture("../assets/clutch.png");
    absTexture = loadTexture("../assets/abs.png");
    tcTexture = loadTexture("../assets/tc.png");
    preRenderBackground();
    bgRect = {0, 0, width, height};
}

void Renderer::render(const VehicleData& data, float speed){
    smoothedRpm = smoothingFactor * data.engineRpm + (1 - smoothingFactor) * smoothedRpm;
    smoothedLoad = smoothingFactor * data.engineLoad + (1 - smoothingFactor) * smoothedLoad;
    smoothedThrottle = smoothingFactor * data.throttle + (1 - smoothingFactor) * smoothedThrottle;

    SDL_SetRenderTarget(renderer, renderTexture);

    SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer, renderedBackgroundTexture, NULL, &bgRect);

    renderGear(data.currentGear);
    renderGear(data.gearGoal, true);
    renderSpeed(speed);
    renderRPM();
    renderLoadThrottleBars();
    renderInfoTexts(data.ambientTemp, data.coolantTemp, data.voltage, data.clutchPressed);

    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopyEx(renderer, renderTexture, nullptr, &bgRect, screenAngle, nullptr, SDL_FLIP_NONE);

    SDL_RenderPresent(renderer);
}

void Renderer::renderLoadThrottleBar(float startAngle, float endAngle, SDL_Color color, bool outline) {
    SDL_Color barBackColor = {50, 50, 50, 100};
    int numPoints = 100;
    int customRadius = radius + 120;
    int customInnerRadius = radius + 55;
    std::vector<Sint16> vX(numPoints);
    std::vector<Sint16> vY(numPoints);
    if(outline) {
        generateArcPoints(startAngle, endAngle, customRadius, customInnerRadius, vX, vY, true);
        polygonRGBA(renderer, vX.data(), vY.data(), numPoints, color.r, color.g, color.b, color.a);
        filledPolygonRGBA(renderer, vX.data(), vY.data(), numPoints, barBackColor.r, barBackColor.g, barBackColor.b, barBackColor.a);
    }else{
        generateArcPoints(startAngle, endAngle, customRadius, customInnerRadius, vX, vY);
        filledPolygonRGBA(renderer, vX.data(), vY.data(), numPoints, color.r, color.g, color.b, color.a);
    }
}

static SDL_Color lerpColor(SDL_Color c1, SDL_Color c2, float t) {
    SDL_Color result;
    result.r = (Uint8)(c1.r + (c2.r - c1.r) * t);
    result.g = (Uint8)(c1.g + (c2.g - c1.g) * t);
    result.b = (Uint8)(c1.b + (c2.b - c1.b) * t);
    result.a = (Uint8)(c1.a + (c2.a - c1.a) * t);
    return result;
}

void Renderer::renderLoadThrottleBars() {
    SDL_Color loadColor;
    float loadRatio = smoothedLoad / 100.0f;
    loadRatio = std::max(0.0f, std::min(1.0f, loadRatio));
    SDL_Color green = {0, 255, 0, 255};
    SDL_Color yellow = {255, 255, 0, 255};
    SDL_Color red = {255, 0, 0, 255};

    if (loadRatio < 0.7f) {
        float t = loadRatio / 0.7f;
        loadColor = lerpColor(green, yellow, t);
    } else {
        float t = (loadRatio - 0.7f) / 0.3f;
        loadColor = lerpColor(yellow, red, t);
    }

    loadColor.a = 140;

    renderLoadThrottleBar(LOAD_ANGLE_END, LOAD_ANGLE_START + (LOAD_ANGLE_END - LOAD_ANGLE_START) * (1.0 - smoothedLoad / 100.0f), loadColor, false);
    renderLoadThrottleBar(THROTTLE_ANGLE_START, THROTTLE_ANGLE_START + (THROTTLE_ANGLE_END - THROTTLE_ANGLE_START) * smoothedThrottle / THROTTLE_MAX, {20, 20, 255, 140}, false);

    SDL_Rect loadTextureRect = {80, height - 65, 60, 60};
    SDL_Color engineLoadColor = smoothedLoad > 80.0f ? SDL_Color{255, 255, 20, 255} : SDL_Color{255, 255, 255, 255};
    SDL_SetTextureColorMod(engineLoadTexture, engineLoadColor.r, engineLoadColor.g, engineLoadColor.b);
    SDL_RenderCopy(renderer, engineLoadTexture, NULL, &loadTextureRect);
    SDL_Rect throttleTextureRect = {width - 80 - 50, height - 60, 60, 60};
    SDL_RenderCopy(renderer, throttleTexture, NULL, &throttleTextureRect);
}

void Renderer::renderGear(int gear, bool goal) {
    if(goal && gear == GEAR_NONE){
        return;
    }
    std::string gearText = gear == 0 ? "N" : std::to_string(gear);
    SDL_Color outlineColor = {216, 67, 21, 255};
    SDL_Color fillColor = {0, 0, 0, 255};
    SDL_Surface* gearSurface = TTF_RenderText_Blended(goal ? gearGoalFont : gearFont, gearText.c_str(), {255,255,255,255});
    SDL_Texture* gearTexture = SDL_CreateTextureFromSurface(renderer, gearSurface);
    SDL_Rect gearRect = {
        goal ? 250 : (width - gearSurface->w) / 2,
        centerY - gearSurface->h / 2,
        gearSurface->w,
        gearSurface->h
    };

    SDL_SetTextureColorMod(gearTexture, outlineColor.r, outlineColor.g, outlineColor.b);
    for (int dx = -2; dx <= 5; ++dx) {
        for (int dy = -2; dy <= 5; ++dy) {
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

void Renderer::renderSpeed(float speed) {
    int intSpeed = static_cast<int>(speed);
    std::ostringstream speedOss;
    speedOss.width(2);
    speedOss.fill('0');
    speedOss << intSpeed;
    std::string speedText = speedOss.str();
    const SDL_Color speedColor = {255, 255, 255, 255};
    SDL_Surface* speedSurface = TTF_RenderText_Blended(speedFont, speedText.c_str(), speedColor);
    SDL_Texture* speedTexture = SDL_CreateTextureFromSurface(renderer, speedSurface);
    SDL_Rect speedRect = {
        (width - speedSurface->w) / 2,
        centerY + speedSurface->h / 2 + 20,
        speedSurface->w,
        speedSurface->h
    };
    SDL_RenderCopyEx(renderer, speedTexture, nullptr, &speedRect, 0, nullptr, SDL_FLIP_NONE);
    SDL_FreeSurface(speedSurface);
    SDL_DestroyTexture(speedTexture);
}

void Renderer::renderRPM() {
    Uint8 a = 160;
    SDL_Color lightBlue = {20, 20, 230, a};
    SDL_Color orange = {216, 67, 21, a};
    SDL_Color red = {255, 20, 20, 200};
    float rpm = smoothedRpm / 1000.0f;
    SDL_Color rpmColor;

    if (rpm <= 6.0f) {
        rpmColor = lightBlue;
    } else if (rpm <= 9.0f) {
        float t = (rpm - 6.0f) / 3.0f;
        rpmColor = lerpColor(lightBlue, orange, t);
    } else if (rpm <= 9.5f) {
        rpmColor = orange;
    } else if (rpm <= 12.0f) {
        float t = (rpm - 9.5f) / 2.5f;
        rpmColor = lerpColor(orange, red, t);
    } else {
        rpmColor = red;
    }

    float rpmRatio = smoothedRpm / RPM_MAX;
    drawRPMArc(RPM_ARC_END_ANGLE, RPM_ARC_START_ANGLE - (RPM_ARC_START_ANGLE - RPM_ARC_END_ANGLE) * (1.0 - rpmRatio), rpmColor, false);
    drawRPMNumbers();
    drawNeedle(rpmRatio);
}

void Renderer::drawRPMArc(float startAngle, float endAngle, SDL_Color color, bool ticks) {
    int numPoints = 120;
    float angleRange = startAngle - endAngle;

    std::vector<Sint16> vX(numPoints);
    std::vector<Sint16> vY(numPoints);
    generateArcPoints(startAngle, endAngle, radius, innerRadius, vX, vY, ticks);

    filledPolygonRGBA(renderer, vX.data(), vY.data(), numPoints, color.r, color.g, color.b, color.a);

    if (!ticks){
        return;
    }

    polygonRGBA(renderer, vX.data(), vY.data(), numPoints, 255, 255, 255, 200);

    int numTicks = 24;
    int tickLength = 18;
    int tickRadius = radius - 1;
    int tickThickness = 4;
    for (int i = 0; i <= numTicks; ++i) {
        if (i == 0 || i == numTicks){
            continue;
        }
        int customTickLength = tickLength;
        int customTickThickness = tickThickness;
        if (i % 2 == 1) {
            customTickLength = 12;
            customTickThickness = 3;
        }
        float tickAngle = startAngle - ((float)i / (float)numTicks) * angleRange;
        float tickAngleRad = tickAngle * M_PI / 180.0f;
        int tickOuterX = centerX - tickRadius * cosf(tickAngleRad);
        int tickOuterY = centerY + tickRadius * sinf(tickAngleRad);
        int tickInnerX = centerX - (tickRadius - customTickLength) * cosf(tickAngleRad);
        int tickInnerY = centerY + (tickRadius - customTickLength) * sinf(tickAngleRad);
        thickLineRGBA(renderer, tickOuterX, tickOuterY, tickInnerX, tickInnerY, customTickThickness, 170, 170, 170, 220);
    }
}

void Renderer::drawRPMNumbers() {
    const int numNumbers = 12;
    const float angleStep = (RPM_ARC_END_ANGLE - RPM_ARC_START_ANGLE) / numNumbers;
    const int numberRadius = innerRadius + (radius - innerRadius) / 2.7;

    for (int i = 0; i <= numNumbers; ++i) {
        float angle = RPM_ARC_END_ANGLE - i * angleStep;
        float angleRad = angle * M_PI / 180.0f;
        int x = centerX - numberRadius * cosf(angleRad);
        int y = centerY + numberRadius * sinf(angleRad);
        std::string numberText = std::to_string(i);
        SDL_Color numberColor;

        if (i == 11 || i == 12) {
            numberColor = {255, 0, 0, 255};
        } else {
            numberColor = {255, 255, 255, 255};
        }

        SDL_Surface* numberSurface = TTF_RenderText_Blended(numberFont, numberText.c_str(), numberColor);
        SDL_Texture* numberTexture = SDL_CreateTextureFromSurface(renderer, numberSurface);
        SDL_Rect numberRect = {x - numberSurface->w / 2 + 2, y - numberSurface->h / 2 + 8, numberSurface->w, numberSurface->h};
        SDL_Color outlineColor = {0, 0, 0, 255};
        SDL_SetTextureColorMod(numberTexture, outlineColor.r, outlineColor.g, outlineColor.b);

        for (int offsetX = -2; offsetX <= 2; ++offsetX) {
            for (int offsetY = -2; offsetY <= 2; ++offsetY) {
                SDL_Rect outlineRect = numberRect;
                outlineRect.x += offsetX;
                outlineRect.y += offsetY;
                SDL_RenderCopy(renderer, numberTexture, NULL, &outlineRect);
            }
        }

        SDL_SetTextureColorMod(numberTexture, numberColor.r, numberColor.g, numberColor.b);
        SDL_RenderCopy(renderer, numberTexture, NULL, &numberRect);
        SDL_FreeSurface(numberSurface);
        SDL_DestroyTexture(numberTexture);
    }
}

void Renderer::drawNeedle(float rpmRatio) {
    float angle = RPM_ARC_START_ANGLE - (RPM_ARC_START_ANGLE - RPM_ARC_END_ANGLE) * (1.0 - rpmRatio);
    float angleRad = angle * M_PI / 180.0f;
    int startX = centerX - innerRadius * cosf(angleRad);
    int startY = centerY + innerRadius * sinf(angleRad);
    int endX = centerX - radius * cosf(angleRad);
    int endY = centerY + radius * sinf(angleRad);
    SDL_Color needleColor = {255, 0, 0, 255};
    thickLineRGBA(renderer, startX, startY, endX, endY, 3, needleColor.r, needleColor.g, needleColor.b, needleColor.a);
}

SDL_Texture* Renderer::loadTexture(const std::string& filePath) {
    SDL_Texture* texture = IMG_LoadTexture(renderer, filePath.c_str());

    if (!texture) {
        std::cerr << "IMG_LoadTexture Error: " << IMG_GetError() << std::endl;
    }

    return texture;
}

void Renderer::renderInfoTexts(float ambientTemp, float coolantTemp, float batteryVoltage, bool clutchPressed) {
    auto renderIcon = [&](SDL_Texture* iconTexture, int x, int y, int size){
        SDL_Rect iconRect = {x, y, size, size};
        SDL_RenderCopy(renderer, iconTexture, NULL, &iconRect);
    };
    auto renderInfoTextWithIcon = [&](SDL_Texture* iconTexture, int x, int y, float value, const std::string& label, const SDL_Color& color) {
        int iconSize = 32;
        renderIcon(iconTexture, x, y, iconSize);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << value << " " << label;
        std::string text = ss.str();
        SDL_Surface* textSurface = TTF_RenderText_Blended(infoFont, text.c_str(), color);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_Rect textRect = {x + iconSize + 5, y + (iconSize - textSurface->h) / 2, textSurface->w, textSurface->h};
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);
    };
    int iconSize = 32;
    int x = 40;
    int y = 20;
    int yOffset = iconSize + 10;

    SDL_Color batteryColor =
            (batteryVoltage < 11.0) ? SDL_Color{255, 20, 20, 255} :
            (batteryVoltage < 12.0) ? SDL_Color{255, 255, 20, 255} :
            SDL_Color{20, 255, 20, 255};
    SDL_SetTextureColorMod(batteryTexture, batteryColor.r, batteryColor.g, batteryColor.b);
    renderInfoTextWithIcon(batteryTexture, width - 160, y, batteryVoltage, "V", batteryColor);

    renderInfoTextWithIcon(tempTexture, x, y, ambientTemp, "C", SDL_Color{255, 255, 255, 255});

    y += yOffset;

    SDL_Color coolantTempColor =
            (coolantTemp > 60.0f) ? SDL_Color{255, 20, 20, 255} :
            (coolantTemp > 40.0f) ? SDL_Color{255, 255, 20, 255} :
            SDL_Color{20, 255, 20, 255};
    SDL_SetTextureColorMod(coolantTexture, coolantTempColor.r, coolantTempColor.g, coolantTempColor.b);
    renderInfoTextWithIcon(coolantTexture, x, y, coolantTemp, "C", coolantTempColor);

    SDL_Color clutchColor = clutchPressed ? SDL_Color{20, 255, 20, 255} : SDL_Color{255, 255, 255, 255};
    SDL_SetTextureColorMod(clutchTexture, clutchColor.r, clutchColor.g, clutchColor.b);
    renderIcon(clutchTexture, width - 164, y, 40);

    if(smoothedRpm < 6000){
        return;
    }
    SDL_Color warningColor = {255, 255, 20, 255};
    SDL_SetTextureColorMod(absTexture, warningColor.r, warningColor.g, warningColor.b);
    renderIcon(absTexture, width / 2 - 80, height - 66, 80);
    SDL_SetTextureColorMod(tcTexture, warningColor.r, warningColor.g, warningColor.b);
    renderIcon(tcTexture, width / 2 + 20, height - 46, 42);
}

void Renderer::generateArcPoints(float startAngle, float endAngle, int outerRad, int innerRad, std::vector<Sint16>& vX, std::vector<Sint16>& vY, bool outline) const{
    auto arcOffsetAngle = [&](float radius, float offset) {
        if (offset >= radius) return 90.0f;
        return asinf(offset / radius) * 180.0f / (float)M_PI;
    };
    float offsetAmount = outline ? 1.0f : 0.0f;
    float angleOffsetInner = arcOffsetAngle(innerRad, offsetAmount);
    float angleOffsetOuter = arcOffsetAngle(outerRad, offsetAmount);
    float angleOffset = std::min(angleOffsetInner, angleOffsetOuter);

    float newStartAngle = startAngle - angleOffset;
    float newEndAngle   = endAngle   + angleOffset;

    float angleRange = newStartAngle - newEndAngle;
    int numPoints = (int)vX.size() / 2;

    for (int i = 0; i < numPoints; ++i) {
        float angle = newStartAngle - ((float)i / (float)(numPoints - 1)) * angleRange;
        float angleRad = angle * (float)M_PI / 180.0f;
        float r = (float)innerRad - offsetAmount;
        vX[i] = short((float)centerX - r * cosf(angleRad));
        vY[i] = short((float)centerY + r * sinf(angleRad));
    }
    for (int i = 0; i < numPoints; ++i) {
        float angle = newStartAngle - ((float)i / (float)(numPoints - 1)) * angleRange;
        float angleRad = angle * (float)M_PI / 180.0f;
        float r = (float)outerRad + offsetAmount;
        vX[(numPoints * 2) - 1 - i] = short((float)centerX - r * cosf(angleRad));
        vY[(numPoints * 2) - 1 - i] = short((float)centerY + r * sinf(angleRad));
    }
}
