#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <thread>
#include <atomic>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 480;

const SDL_Color TOP_COLOR = {0, 0, 255, 255};       // Blue (top gradient)
const SDL_Color BOTTOM_COLOR = {255, 0, 0, 255};    // Red (bottom gradient)
const SDL_Color TEXT_COLOR = {255, 255, 255, 255};  // White text

std::atomic<int> intakeTemp(20);

void renderGradient(SDL_Renderer* renderer) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        float t = static_cast<float>(y) / SCREEN_HEIGHT;
        Uint8 r = TOP_COLOR.r + (BOTTOM_COLOR.r - TOP_COLOR.r) * t;
        Uint8 g = TOP_COLOR.g + (BOTTOM_COLOR.g - TOP_COLOR.g) * t;
        Uint8 b = TOP_COLOR.b + (BOTTOM_COLOR.b - TOP_COLOR.b) * t;

        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderDrawLine(renderer, 0, y, SCREEN_WIDTH, y);
    }
}

void updateTemperature() {
    while (true) {
        intakeTemp += 1;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main() {
    setenv("SDL_VIDEODRIVER", "kmsdrm", 1);
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow("Instrument Cluster", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 72);
    if (!font) std::cerr << "Failed to load font\n";

    std::thread tempThread(updateTemperature);

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        renderGradient(renderer);

        std::string tempText = "Intake: " + std::to_string(intakeTemp.load()) + "°C";
        SDL_Surface* textSurface = TTF_RenderText_Blended(font, tempText.c_str(), TEXT_COLOR);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

        SDL_Rect textRect{
            (SCREEN_WIDTH - textSurface->w) / 2,
            (SCREEN_HEIGHT - textSurface->h) / 2,
            textSurface->w,
            textSurface->h
        };

        SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);

        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);

        SDL_RenderPresent(renderer);
        SDL_Delay(50);
    }

    tempThread.detach();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}
