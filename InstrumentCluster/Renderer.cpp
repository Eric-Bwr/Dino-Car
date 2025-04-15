#include "Renderer.h"
#include "VehicleConstants.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <SDL2/SDL2_gfxPrimitives.h>

#define IS_RASPI (__arm__ || __aarch64__)
const float START_ANGLE = 90.0f + 30.0f;
const float END_ANGLE = 90.0f + 330.0f;

Renderer::Renderer(int width, int height) : window(nullptr), renderer(nullptr), gearFont(nullptr), speedFont(nullptr), width(width), height(height){
#if IS_RASPI
    screenAngle = 180.0;
#else
    screenAngle = 0.0;
#endif
    centerX = width / 2;
    centerY = height / 2;
    radius = std::min(width, height) / 2.15;
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
    gearFont = TTF_OpenFont("../bebas.ttf", 270);
    speedFont = TTF_OpenFont("../bebas.ttf", 100);
    numberFont = TTF_OpenFont("../bebas.ttf", 40);
}

void Renderer::render(int gear, int rpm, float temp, float speed){
    SDL_SetRenderDrawColor(renderer, 0, 0, 20, 255);
    SDL_RenderClear(renderer);
    renderGear(gear);
    renderSpeed(speed);
    renderRPM(rpm);
    const int numNumbers = 12;
    const float angleStep = (END_ANGLE - START_ANGLE) / numNumbers;
    const int outerTickRadius = radius;
    const int innerTickRadius = radius - 18; // Länge der Einkerbung

    for (int i = 0; i <= numNumbers; ++i) {
        float angle = END_ANGLE - i * angleStep;
        float angleRad = angle * M_PI / 180.0f;

        int x1 = centerX - outerTickRadius * cosf(angleRad);
        int y1 = centerY + outerTickRadius * sinf(angleRad);
        int x2 = centerX - innerTickRadius * cosf(angleRad);
        int y2 = centerY + innerTickRadius * sinf(angleRad);

        // Optional: Für 11 und 12 rote Einkerbung, sonst weiß
        SDL_Color tickColor = (i == 11 || i == 12) ? SDL_Color{255, 0, 0, 255} : SDL_Color{255, 255, 255, 255};

        thickLineRGBA(renderer, x1, y1, x2, y2, 3, tickColor.r, tickColor.g, tickColor.b, tickColor.a);
    }
    SDL_RenderPresent(renderer);
}

void Renderer::renderGear(int gear) {
    std::string gearText = gear == 0 ? "N" : std::to_string(gear);
    const SDL_Color gearColor = {255, 255, 255, 255};
    SDL_Surface* gearSurface = TTF_RenderText_Blended(gearFont, gearText.c_str(), gearColor);
    SDL_Texture* gearTexture = SDL_CreateTextureFromSurface(renderer, gearSurface);
    SDL_Rect gearRect = {
        (width - gearSurface->w) / 2,
        centerY - gearSurface->h / 2,
        gearSurface->w,
        gearSurface->h
    };
    SDL_RenderCopyEx(renderer, gearTexture, nullptr, &gearRect, 0, nullptr, SDL_FLIP_NONE);
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

    SDL_Color rpmColor = {60, 80, 160, 200};

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
    const int numberRadius = innerRadius + (radius - innerRadius) / 2;
    const int tickRadius = radius; // Radius für die Einkerbungen
    const int tickSize = 12; // Größe der Einkerbung (Radius des Kreises)

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
        SDL_RenderCopy(renderer, numberTexture, NULL, &numberRect);

        SDL_FreeSurface(numberSurface);
        SDL_DestroyTexture(numberTexture);

        // Zeichne die Einkerbung (Halbkreis)
        int tickX = centerX - tickRadius * cosf(angleRad);
        int tickY = centerY + tickRadius * sinf(angleRad);
        aacircleRGBA(renderer, tickX, tickY, tickSize, 0, 0, 20, 255);
        filledCircleRGBA(renderer, tickX, tickY, tickSize, 0, 0, 20, 255);
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