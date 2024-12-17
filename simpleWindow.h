#pragma once
#include <functional>
#include <optional>
#include <mutex>
#include <atomic>
#include "vec.h"
#include <opencv2/opencv.hpp>
class simpleWindow
{
private:
    cv::Mat wndMat;
    int nWidth, nHeight;
    int lstWidth, lstHeight;
    bool bPress[128];
    char lastPress;
    std::function<void(int, int)> resizeCallback;
    const char* windowName;
    vec3 bkColor;

public:
    void create(const char *name, int width, int height);
    void show();
    bool shouldClose();
    void setPixel(int x, int y, int r, int g, int b);
    void setPixel(int x, int y, vec3 color);
    void clear();
    void setBkColor(int r, int g, int b);
    bool press(char key);
    char getKey();
    int geiWidth() { return nWidth; }
    int getHeight() { return nHeight; }
    void setResizeCallback(const std::function<void(int, int)> &f) { resizeCallback = f; }
};