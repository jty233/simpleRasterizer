#include "simpleWindow.h"
#include <SDL.h>
#include <string>
#include <iostream>
#include <thread>
#include <future>

using namespace std;

simpleWindow::~simpleWindow()
{
    if (glContext)
        SDL_GL_DeleteContext(glContext);
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

    // 设置OpenGL属性
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window = SDL_CreateWindow(name,
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              width, height,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (!window)
    {
        cerr << "Window could not be created! Error: " << SDL_GetError() << endl;
        return;
    }

    // 创建OpenGL上下文
    glContext = SDL_GL_CreateContext(window);
    if (!glContext)
    {
        cerr << "OpenGL context could not be created! Error: " << SDL_GetError() << endl;
        return;
    }

    // 初始化GLEW
#ifndef __APPLE__
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << endl;
    }
#endif

    // 启用垂直同步
    if (SDL_GL_SetSwapInterval(1) < 0)
    {
        cerr << "Warning: Unable to set VSync! Error: " << SDL_GetError() << endl;
    }

    // 创建用于CPU渲染的渲染器和纹理
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

void simpleWindow::showWithOpenGL(span<uint32_t> data)
{
    // 这里我们将CPU端的数据纹理显示到OpenGL渲染目标上
    // 为简单起见，我们直接交换缓冲区
    SDL_GL_SwapWindow(window);
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
