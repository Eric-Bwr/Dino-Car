#include "Renderer.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>

const float START_ANGLE = 90.0f + 30.0f;
const float END_ANGLE = 90.0f + 330.0f;

Renderer::Renderer(int width, int height) : window(nullptr), renderer(nullptr), gearFont(nullptr), speedFont(nullptr), numberFont(nullptr), width(width), height(height){
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
    speedFont = TTF_OpenFont("../assets/trans.ttf", 100);
    numberFont = TTF_OpenFont("../assets/trans.ttf", 60);
    tempTexture = loadTexture("../assets/temp.png");
    coolantTexture = loadTexture("../assets/coolant.png");
    engineLoadTexture = loadTexture("../assets/load.png");
    batteryTexture = loadTexture("../assets/battery.png");
    throttleTexture = loadTexture("../assets/throttle.png");
}

void Renderer::render(const VehicleData& data, float speed){
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    renderGear(data.currentGear);
    renderSpeed(speed);
    renderRPM(data.currentRpm);

    SDL_Rect destRect;
    destRect.x = 100;
    destRect.y = 100;
    destRect.w = 50;
    destRect.h = 50;
    SDL_RenderCopy(renderer, coolantTexture, NULL, &destRect);


    SDL_Rect destRect3;
    destRect3.x = 120;
    destRect3.y = 50;
    destRect3.w = 50;
    destRect3.h = 50;
    SDL_RenderCopy(renderer, batteryTexture, NULL, &destRect3);
    SDL_Rect destRect4;
    destRect3.x = 120;
    destRect3.y = 300;
    destRect3.w = 50;
    destRect3.h = 50;
    SDL_RenderCopy(renderer, tempTexture, NULL, &destRect3);

    renderLoadThrottleBars(data.currentLoad, data.currentThrottle);

    SDL_RenderPresent(renderer);
}

void Renderer::renderLoadThrottleBars(float load, float throttle) {
    const int barWidth = 80;         // Increased bar width
    const int barHeight = 280;        // Increased bar height
    const int barY = centerY - barHeight / 2 + 20;
    const int roundness = 15;      // Roundness of the corners

    // Load bar
    const int loadBarX = centerX - radius - barWidth - 30;  // Adjust position
    SDL_Color loadColor;
    float loadRatio = load / 100.0f;
    int barOffset = 2;
    loadRatio = std::max(0.0f, std::min(1.0f, loadRatio)); // Clamp the value

    // Smooth color change using linear interpolation
    SDL_Color green = {0, 255, 0, 255};
    SDL_Color yellow = {255, 255, 0, 255};
    SDL_Color red = {255, 0, 0, 255};

    if (loadRatio < 0.7f) {
        // Green to yellow
        float ratio = loadRatio / 0.7f;
        loadColor.r = (Uint8)(green.r * (1 - ratio) + yellow.r * ratio);
        loadColor.g = (Uint8)(green.g * (1 - ratio) + yellow.g * ratio);
        loadColor.b = (Uint8)(green.b * (1 - ratio) + yellow.b * ratio);
    } else {
        // Yellow to red
        float ratio = (loadRatio - 0.7f) / 0.3f;
        loadColor.r = (Uint8)(yellow.r * (1 - ratio) + red.r * ratio);
        loadColor.g = (Uint8)(yellow.g * (1 - ratio) + red.g * ratio);
        loadColor.b = (Uint8)(yellow.b * (1 - ratio) + red.b * ratio);
    }

    roundedBoxRGBA(renderer, loadBarX - barOffset, barY - barOffset, loadBarX + barWidth + barOffset, barY + barHeight + barOffset, roundness, 255, 255, 255, 255);

    int fillHeight = static_cast<int>(barHeight * loadRatio);

    roundedBoxRGBA(renderer, loadBarX, barY + barHeight - fillHeight, loadBarX + barWidth, barY + barHeight, roundness, loadColor.r, loadColor.g, loadColor.b, 255);

    const int throttleBarX = centerX + radius + 30;
    roundedBoxRGBA(renderer, throttleBarX - barOffset, barY - barOffset, throttleBarX + barWidth + barOffset, barY + barHeight + barOffset, roundness, 255, 255, 255, 255);

    // Render rounded rectangle for the throttle bar fill
    int throttleFillHeight = static_cast<int>(barHeight * throttle / 100.0f);
    roundedBoxRGBA(renderer, throttleBarX, barY + barHeight - throttleFillHeight, throttleBarX + barWidth, barY + barHeight, roundness, 0, 0, 255, 255);

    const int textureWidth = 60;       // Increased texture size
    const int textureHeight = 60;      // Increased texture size
    const int textureY = barY + barHeight + 10;

    // Load texture
    SDL_Rect loadTextureRect = {loadBarX - (textureWidth - barWidth) / 2, textureY, textureWidth, textureHeight};
    SDL_RenderCopy(renderer, engineLoadTexture, NULL, &loadTextureRect);

    // Throttle texture
    SDL_Rect throttleTextureRect = {throttleBarX - (textureWidth - barWidth) / 2, textureY, textureWidth, textureHeight};
    SDL_RenderCopy(renderer, throttleTexture, NULL, &throttleTextureRect);
}
void Renderer::renderGear(int gear) {
    std::string gearText = gear == 0 ? "N" : std::to_string(gear);
    SDL_Color outlineColor = {216, 67, 21, 255}; // Your orange/red outline
    SDL_Color fillColor = {0, 0, 0, 255};        // Black inside

    // Render the text surface in white (we'll modulate color)
    SDL_Surface* gearSurface = TTF_RenderText_Blended(gearFont, gearText.c_str(), {255,255,255,255});
    SDL_Texture* gearTexture = SDL_CreateTextureFromSurface(renderer, gearSurface);
    SDL_Rect gearRect = {
        (width - gearSurface->w) / 2,
        centerY - gearSurface->h / 2,
        gearSurface->w,
        gearSurface->h
    };

    // Draw the outline (offsets around the center)
    SDL_SetTextureColorMod(gearTexture, outlineColor.r, outlineColor.g, outlineColor.b);
    for (int dx = -2; dx <= 5; ++dx) {
        for (int dy = -2; dy <= 5; ++dy) {
            if (dx == 0 && dy == 0) continue; // skip center
            SDL_Rect outlineRect = gearRect;
            outlineRect.x += dx;
            outlineRect.y += dy;
            SDL_RenderCopy(renderer, gearTexture, nullptr, &outlineRect);
        }
    }

    // Draw the inside (center) in black
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
        centerY + speedSurface->h / 2 + 50,
        speedSurface->w,
        speedSurface->h
    };
    SDL_RenderCopyEx(renderer, speedTexture, nullptr, &speedRect, 0, nullptr, SDL_FLIP_NONE);
    SDL_FreeSurface(speedSurface);
    SDL_DestroyTexture(speedTexture);
}

void Renderer::renderRPM(int rpm) {
    float rpmRatio = static_cast<float>(rpm) / RPM_MAX;

    SDL_Color rpmColor = {216, 67, 21, 200};

    SDL_Color rpmBackColor = {50, 50, 50, 255};

    drawRPMArc(START_ANGLE, END_ANGLE, rpmBackColor);
    drawRPMArc(END_ANGLE, START_ANGLE - (START_ANGLE - END_ANGLE) * (1.0 - rpmRatio), rpmColor);
    drawRPMNumbers();
    drawNeedle(rpmRatio);
}

void Renderer::drawRPMArc(float startAngle, float endAngle, SDL_Color color) {
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
}

void Renderer::drawRPMNumbers() {
    const int numNumbers = 12;
    const float angleStep = (END_ANGLE - START_ANGLE) / numNumbers;
    const int numberRadius = innerRadius + (radius - innerRadius) / 2.05;

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

        SDL_Rect numberRect = {x - numberSurface->w / 2, y - numberSurface->h / 2, numberSurface->w, numberSurface->h};

        // Render the outline of the text
        SDL_Color outlineColor = {0, 0, 0, 255};  // Black outline color
        SDL_SetTextureColorMod(numberTexture, outlineColor.r, outlineColor.g, outlineColor.b);
        for (int offsetX = -2; offsetX <= 2; ++offsetX) {
            for (int offsetY = -2; offsetY <= 2; ++offsetY) {
                SDL_Rect outlineRect = numberRect;
                outlineRect.x += offsetX;
                outlineRect.y += offsetY;
                SDL_RenderCopy(renderer, numberTexture, NULL, &outlineRect);
            }
        }

        // Render the actual text
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