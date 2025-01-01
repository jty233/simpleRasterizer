#include "simpleWindow.h"
#include <SDL.h>
#include <string>
#include <iostream>
#include <thread>
#include <future>

using namespace std;

simpleWindow::~simpleWindow()
{
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void simpleWindow::create(const char *name, int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cerr << "SDL could not initialize! Error: " << SDL_GetError() << endl;
        return;
    }

    window = SDL_CreateWindow(name,
                             SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED,
                             width, height,
                             SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        cerr << "Window could not be created! Error: " << SDL_GetError() << endl;
        return;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        cerr << "Renderer could not be created! Error: " << SDL_GetError() << endl;
        return;
    }

    texture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               width, height);
    if (!texture) {
        cerr << "Texture could not be created! Error: " << SDL_GetError() << endl;
        return;
    }

    pixels.resize(width * height);
    lstWidth = nWidth = width;
    lstHeight = nHeight = height;

    clear();
    show();
}

void simpleWindow::show()
{
    SDL_UpdateTexture(texture, nullptr, pixels.data(), nWidth * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                shouldCloseFlag = true;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    lstWidth = event.window.data1;
                    lstHeight = event.window.data2;
                    if (resizeCallback) {
                        resizeCallback(lstWidth, lstHeight);
                    }
                }
                break;
            case SDL_KEYDOWN:
                lastPress = event.key.keysym.sym;
                break;
        }
    }
}

bool simpleWindow::shouldClose()
{
    return shouldCloseFlag;
}

void simpleWindow::setPixel(int x, int y, int r, int g, int b)
{
    if (x < 0 || x >= nWidth || y < 0 || y >= nHeight) return;
    y = nHeight - y - 1;
    pixels[y * nWidth + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
}

void simpleWindow::setPixel(int x, int y, vec3 color)
{
    setPixel(x, y, (int)color[0], (int)color[1], (int)color[2]);
}

void simpleWindow::clear()
{
    uint32_t color = (0xFF << 24) | 
                    ((uint8_t)bkColor[0] << 16) | 
                    ((uint8_t)bkColor[1] << 8) | 
                    (uint8_t)bkColor[2];
    fill(pixels.begin(), pixels.end(), color);
}

void simpleWindow::setBkColor(int r, int g, int b)
{
    bkColor = vec3(r, g, b);
    clear();
}

bool simpleWindow::press(char key)
{
    return bPress[key];
}

char simpleWindow::getKey()
{
    char key = lastPress;
    lastPress = 0;
    return key;
}
