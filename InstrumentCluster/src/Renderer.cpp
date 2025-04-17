#include "Renderer.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>

const float START_ANGLE = 90.0f + 30.0f;
const float END_ANGLE = 90.0f + 330.0f;

Renderer::Renderer(int width, int height) : window(nullptr), renderer(nullptr), width(width), height(height){
#if IS_RASPI
    screenAngle = 180.0;
#else
    screenAngle = 0.0;
#endif
    centerX = width / 2;
    centerY = height / 2;
    radius = std::min(width, height) / 2;
    innerRadius = radius - 70;
    centerY += 10;
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
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    gearFont = TTF_OpenFont("../assets/trans.ttf", 270);
    speedFont = TTF_OpenFont("../assets/bebas.ttf", 100);
    numberFont = TTF_OpenFont("../assets/trans.ttf", 60);
    infoFont = TTF_OpenFont("../assets/bebas.ttf", 40);
    bgTexture = loadTexture("../assets/bg.png");
    tempTexture = loadTexture("../assets/temp.png");
    coolantTexture = loadTexture("../assets/coolant.png");
    engineLoadTexture = loadTexture("../assets/load.png");
    batteryTexture = loadTexture("../assets/battery.png");
    throttleTexture = loadTexture("../assets/throttle.png");
    clutchTexture = loadTexture("../assets/clutch.png");
}

void Renderer::render(const VehicleData& data, float speed){
    smoothedRpm = smoothingFactor * data.engineRpm + (1 - smoothingFactor) * smoothedRpm;
    smoothedLoad = smoothingFactor * data.engineLoad + (1 - smoothingFactor) * smoothedLoad;
    smoothedThrottle = smoothingFactor * data.throttle + (1 - smoothingFactor) * smoothedThrottle;

    SDL_RenderClear(renderer);
    SDL_Rect bgRect = {0, 0, width, height};
    SDL_RenderCopy(renderer, bgTexture, NULL, &bgRect);
    renderGear(data.currentGear);
    renderSpeed(speed);
    renderRPM(static_cast<int>(smoothedRpm));
    renderInfoTexts(data.ambientTemp, data.coolantTemp, data.voltage, data.clutchPressed);
    renderLoadThrottleBars(smoothedLoad, smoothedThrottle);
    SDL_RenderPresent(renderer);
}

void Renderer::renderLoadThrottleBars(float load, float throttle) {
    const int barWidth = 80;
    const int barHeight = 280;
    const int barY = centerY - barHeight / 2 + 20;
    const int roundness = 15;
    const int loadBarX = centerX - radius - barWidth - 30;
    SDL_Color loadColor;
    float loadRatio = load / 100.0f;
    int barOffset = 2;
    loadRatio = std::max(0.0f, std::min(1.0f, loadRatio));
    SDL_Color green = {0, 255, 0, 255};
    SDL_Color yellow = {255, 255, 0, 255};
    SDL_Color red = {255, 0, 0, 255};

    if (loadRatio < 0.7f) {
        float ratio = loadRatio / 0.7f;
        loadColor.r = (Uint8)(green.r * (1 - ratio) + yellow.r * ratio);
        loadColor.g = (Uint8)(green.g * (1 - ratio) + yellow.g * ratio);
        loadColor.b = (Uint8)(green.b * (1 - ratio) + yellow.b * ratio);
    } else {
        float ratio = (loadRatio - 0.7f) / 0.3f;
        loadColor.r = (Uint8)(yellow.r * (1 - ratio) + red.r * ratio);
        loadColor.g = (Uint8)(yellow.g * (1 - ratio) + red.g * ratio);
        loadColor.b = (Uint8)(yellow.b * (1 - ratio) + red.b * ratio);
    }

    roundedBoxRGBA(renderer, loadBarX - barOffset, barY - barOffset, loadBarX + barWidth + barOffset, barY + barHeight + barOffset, roundness, 20, 20, 20, 200);
    int fillHeight = static_cast<int>(barHeight * loadRatio);
    roundedBoxRGBA(renderer, loadBarX, barY + barHeight - fillHeight, loadBarX + barWidth, barY + barHeight, roundness, loadColor.r, loadColor.g, loadColor.b, 200);

    const int throttleBarX = centerX + radius + 30;
    roundedBoxRGBA(renderer, throttleBarX - barOffset, barY - barOffset, throttleBarX + barWidth + barOffset, barY + barHeight + barOffset, roundness, 20, 20, 20, 200);
    int throttleFillHeight = static_cast<int>(barHeight * throttle / 80.0f);
    roundedBoxRGBA(renderer, throttleBarX, barY + barHeight - throttleFillHeight, throttleBarX + barWidth, barY + barHeight, roundness, 0, 0, 255, 200);

    const int textureWidth = 60;
    const int textureHeight = 60;
    const int textureY = barY + barHeight + 10;
    SDL_Rect loadTextureRect = {loadBarX - (textureWidth - barWidth) / 2, textureY, textureWidth, textureHeight};
    SDL_RenderCopy(renderer, engineLoadTexture, NULL, &loadTextureRect);
    SDL_Rect throttleTextureRect = {throttleBarX - (textureWidth - barWidth) / 2, textureY, textureWidth, textureHeight};
    SDL_RenderCopy(renderer, throttleTexture, NULL, &throttleTextureRect);
}

void Renderer::renderGear(int gear) {
    std::string gearText = gear == 0 ? "N" : std::to_string(gear);
    SDL_Color outlineColor = {216, 67, 21, 255};
    SDL_Color fillColor = {0, 0, 0, 255};
    SDL_Surface* gearSurface = TTF_RenderText_Blended(gearFont, gearText.c_str(), {255,255,255,255});
    SDL_Texture* gearTexture = SDL_CreateTextureFromSurface(renderer, gearSurface);
    SDL_Rect gearRect = {
        (width - gearSurface->w) / 2,
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

void Renderer::renderRPM(int rpm) {
    float rpmRatio = static_cast<float>(rpm) / RPM_MAX;
    SDL_Color rpmColor = {216, 67, 21, 150};
    SDL_Color rpmBackColor = {50, 50, 50, 100};

    drawRPMArc(START_ANGLE, END_ANGLE, rpmBackColor, true);
    drawRPMArc(END_ANGLE, START_ANGLE - (START_ANGLE - END_ANGLE) * (1.0 - rpmRatio), rpmColor, false);
    drawRPMNumbers();
    drawNeedle(rpmRatio);
}

void Renderer::drawRPMArc(float startAngle, float endAngle, SDL_Color color, bool ticks) {
    int numPoints = 1000;
    Sint16 outerVx[numPoints];
    Sint16 outerVy[numPoints];
    Sint16 innerVx[numPoints];
    Sint16 innerVy[numPoints];
    float angleRange = startAngle - endAngle;

    for (int i = 0; i < numPoints; ++i) {
        float angle = startAngle - ((float)i / (float)(numPoints - 1)) * angleRange;
        float angleRad = angle * M_PI / 180.0f;
        outerVx[i] = centerX - radius * cosf(angleRad);
        outerVy[i] = centerY + radius * sinf(angleRad);
    }

    for (int i = 0; i < numPoints; ++i) {
        float angle = startAngle - ((float)i / (float)(numPoints - 1)) * angleRange;
        float angleRad = angle * M_PI / 180.0f;
        innerVx[i] = centerX - innerRadius * cosf(angleRad);
        innerVy[i] = centerY + innerRadius * sinf(angleRad);
    }

    Sint16 allVx[2 * numPoints];
    Sint16 allVy[2 * numPoints];

    for (int i = 0; i < numPoints; ++i) {
        allVx[i] = outerVx[i];
        allVy[i] = outerVy[i];
        allVx[numPoints + i] = innerVx[numPoints - 1 - i];
        allVy[numPoints + i] = innerVy[numPoints - 1 - i];
    }

    filledPolygonRGBA(renderer, allVx, allVy, 2 * numPoints, color.r, color.g, color.b, color.a);

    if (!ticks){
        return;
    }

    int numTicks = 24;
    for (int i = 0; i <= numTicks; ++i) {
        float tickAngle = startAngle - ((float)i / (float)numTicks) * angleRange;
        float tickAngleRad = tickAngle * M_PI / 180.0f;
        int tickOuterX = centerX - radius * cosf(tickAngleRad);
        int tickOuterY = centerY + radius * sinf(tickAngleRad);
        int tickInnerX = centerX - (radius - 10) * cosf(tickAngleRad);
        int tickInnerY = centerY + (radius - 10) * sinf(tickAngleRad);
        thickLineRGBA(renderer, tickOuterX, tickOuterY, tickInnerX, tickInnerY, 4, 160, 160, 160, 200);
    }
}

void Renderer::drawRPMNumbers() {
    const int numNumbers = 12;
    const float angleStep = (END_ANGLE - START_ANGLE) / numNumbers;
    const int numberRadius = innerRadius + (radius - innerRadius) / 2.0;

    for (int i = 0; i <= numNumbers; ++i) {
        float angle = END_ANGLE - i * angleStep;
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
        SDL_Rect numberRect = {x - numberSurface->w / 2, y - numberSurface->h / 2 + 8, numberSurface->w, numberSurface->h};
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
    float angle = START_ANGLE - (START_ANGLE - END_ANGLE) * (1.0 - rpmRatio);
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
            SDL_Color{255, 255, 255, 255};
    SDL_SetTextureColorMod(coolantTexture, coolantTempColor.r, coolantTempColor.g, coolantTempColor.b);
    renderInfoTextWithIcon(coolantTexture, x, y, coolantTemp, "C", coolantTempColor);

    SDL_Color clutchColor = clutchPressed ? SDL_Color{20, 255, 20, 255} : SDL_Color{255, 255, 255, 255};
    SDL_SetTextureColorMod(clutchTexture, clutchColor.r, clutchColor.g, clutchColor.b);
    renderIcon(clutchTexture, width - 80, y, 40);
}
