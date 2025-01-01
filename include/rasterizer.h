#pragma once
#include "Matrix.h"
#include "model.h"
#include "vec.h"
#include "camera.h"
#include "lightShader.h"
#include "ThreadPool.h"
#include <functional>
#include <atomic>
#include <span>

class rasterizer
{
private:
    int width, height;
    bool resize;
    uint32_t bkColor;
    camera *pCam = nullptr;
    std::deque<std::atomic<float>> zBuffer;
    std::vector<uint32_t> frameBuffer;
    lightShader lig;
    ThreadPool &poolIns;
    std::vector<std::reference_wrapper<model>> models;

    void clearBuffer();
    void drawTriangle(Triangle tri, Triangle ctri, const model &mod,
                     int startX, int startY, int endX, int endY);
    void setPixel(int x, int y, int r, int g, int b);

public:
    rasterizer(int width, int height);
    void setRasterizeSize(int width, int height);
    std::span<uint32_t> draw();
    void setCamera(camera &cam) { pCam = &cam; }
    void addLight(vec3 pos);
    void drawLine(Point begin, Point end, vec3 lineColor = {255, 255, 255});
    void pushModel(model &m) { models.push_back(m); }
    void setBkColor(int r, int g, int b);
};
