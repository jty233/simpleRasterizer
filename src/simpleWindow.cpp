#include "simpleWindow.h"
#include <SDL.h>
#include <string>
#include <iostream>
#include <thread>
#include <future>

using namespace std;

simpleWindow::~simpleWindow()
{
    if (texture)
        SDL_DestroyTexture(texture);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
}

void simpleWindow::create(const char *name, int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        cerr << "SDL could not initialize! Error: " << SDL_GetError() << endl;
        return;
    }

    window = SDL_CreateWindow(name,
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              width, height,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        cerr << "Window could not be created! Error: " << SDL_GetError() << endl;
        return;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        cerr << "Renderer could not be created! Error: " << SDL_GetError() << endl;
        return;
    }

    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                width, height);
    if (!texture)
    {
        cerr << "Texture could not be created! Error: " << SDL_GetError() << endl;
        return;
    }

    nWidth = width;
    nHeight = height;
}

void simpleWindow::show(span<uint32_t> data)
{
    SDL_RenderClear(renderer);
    SDL_UpdateTexture(texture, nullptr, data.data(), nWidth * sizeof(uint32_t));
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

bool simpleWindow::shouldClose()
{
    return shouldCloseFlag;
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

optional<pair<int,int>> simpleWindow::processWindowEvent()
{
    bool resize = false;
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            shouldCloseFlag = true;
            break;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                resize = true;
                nWidth = event.window.data1;
                nHeight = event.window.data2;

                if (texture)
                {
                    SDL_DestroyTexture(texture);
                    texture = nullptr;
                }

                SDL_RenderClear(renderer);

                texture = SDL_CreateTexture(renderer,
                                            SDL_PIXELFORMAT_ARGB8888,
                                            SDL_TEXTUREACCESS_STREAMING,
                                            nWidth, nHeight);
                if (!texture)
                {
                    cerr << "Failed to recreate texture! Error: " << SDL_GetError() << endl;
                    return nullopt;
                }
            }
            break;
        case SDL_KEYDOWN:
            lastPress = event.key.keysym.sym;
            break;
        }
    }
    if (!resize)
        return nullopt;
    
    return make_optional<pair<int,int>>({nWidth, nHeight});
}
