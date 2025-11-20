#pragma once

#include "Matrix.h"
#include "model.h"
#include "vec.h"
#include "camera.h"
#include "lightShader.h"
#include <vector>
#include <span>
#include <SDL.h>

class GPURasterizer
{
private:
    int width, height;
    bool resize;
    uint32_t bkColor;
    camera *pCam = nullptr;
    lightShader lig;
    std::vector<std::reference_wrapper<model>> models;

    // 用于模拟GPU渲染的缓冲区
    std::vector<uint32_t> frameBuffer;
    std::vector<float> zBuffer;

    void clearBuffer();
    void renderScene();

public:
    GPURasterizer(int width, int height);
    ~GPURasterizer();
    void setRasterizeSize(int _width, int _height);
    std::span<uint32_t> draw();
    void setCamera(camera &cam) { pCam = &cam; }
    void addLight(vec3 pos);
    void pushModel(model &m);
    void setBkColor(int r, int g, int b);
};