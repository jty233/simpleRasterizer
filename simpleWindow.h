#pragma once
#include "vec.h"
#include <SDL.h>
#include <vector>
#include <span>
#include <optional>

class simpleWindow
{
private:
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *texture = nullptr;
    bool shouldCloseFlag = false;
    int nWidth, nHeight;
    bool bPress[128];
    char lastPress;
    const char *windowName;

public:
    ~simpleWindow();
    void create(const char *name, int width, int height);
    void show(std::span<uint32_t> data);
    bool shouldClose();
    bool press(char key);
    char getKey();
    std::optional<std::pair<int,int>> processWindowEvent();
};
