#pragma once
#include "vec.h"
#include <SDL.h>
#include <vector>
#include <span>
#include <optional>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

class simpleWindow
{
private:
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *texture = nullptr;
    SDL_GLContext glContext = nullptr;  // OpenGL上下文
    bool shouldCloseFlag = false;
    int nWidth, nHeight;
    bool bPress[128];
    char lastPress;
    const char *windowName;

public:
    ~simpleWindow();
    void create(const char *name, int width, int height);
    void show(std::span<uint32_t> data);
    void showWithOpenGL(std::span<uint32_t> data);  // 用于GPU渲染器的显示方法
    SDL_Window* getSDLWindow() { return window; }  // 获取SDL窗口指针用于创建OpenGL上下文
    SDL_GLContext getGLContext() { return glContext; }  // 获取OpenGL上下文
    bool shouldClose();
    bool press(char key);
    char getKey();
    std::optional<std::pair<int,int>> processWindowEvent();
};
