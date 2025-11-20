#include "GPURasterizer.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <span>
#include <limits>

void GPURasterizer::clearBuffer()
{
    int sz = width * height;
    zBuffer.resize(sz);
    std::fill_n(zBuffer.begin(), sz, std::numeric_limits<float>::infinity());
    frameBuffer.assign(sz, bkColor);
}

void GPURasterizer::renderScene()
{
    clearBuffer();

    Matrix viewpointMatrix;
    Matrix vpv;
    
    if (pCam) {
        pCam->resize(width, height);
        viewpointMatrix = {
            {width / 2., 0, 0, width / 2.},
            {0, height / 2., 0, height / 2.},
            {0, 0, 1, 0},
            {0, 0, 0, 1}
        };
        vpv = viewpointMatrix * pCam->projectionMatrix * pCam->viewMatrix;
    }

    for (auto mref : models) {
        model &m = mref.get();
        Matrix mvpv, mv;
        if (pCam) {
            mvpv = vpv * m.modelMatrix;
            mv = pCam->viewMatrix * m.modelMatrix;
        } else {
            mvpv = viewpointMatrix * m.modelMatrix;
            mv = m.modelMatrix;
        }

        for (Triangle ctri : m.tris) {
            Triangle tri = (mvpv * ctri).normalize();
            Triangle test = mvpv * ctri;
            ctri = (mv * ctri).normalize();

            vec3 ab(ctri.getVertex(1)[0] - ctri.getVertex(0)[0], 
                    ctri.getVertex(1)[1] - ctri.getVertex(0)[1], 
                    ctri.getVertex(1)[2] - ctri.getVertex(0)[2]);
            vec3 ac(ctri.getVertex(2)[0] - ctri.getVertex(0)[0], 
                    ctri.getVertex(2)[1] - ctri.getVertex(0)[1], 
                    ctri.getVertex(2)[2] - ctri.getVertex(0)[2]);
            vec3 nor = (ab.cross(ac)).normalize();
            for (int i = 0; i < 3; i++) {
                if (tri.normal[i])
                    tri.normal[i] = (mv * tri.normal[i].value()).normalize();
                else
                    tri.normal[i] = nor;
            }
            
            // 三角形光栅化
            double ax = tri.getVertex(0)[0], ay = tri.getVertex(0)[1];
            double bx = tri.getVertex(1)[0], by = tri.getVertex(1)[1];
            double cx = tri.getVertex(2)[0], cy = tri.getVertex(2)[1];

            int minx = std::min({int(ax), int(bx), int(cx)});
            int maxx = std::max({int(ax), int(bx), int(cx)});
            int miny = std::min({int(ay), int(by), int(cy)});
            int maxy = std::max({int(ay), int(by), int(cy)});

            int endx = std::min(width, maxx + 1);
            int startx = std::max(0, minx);
            int endy = std::min(height, maxy + 1);
            int starty = std::max(0, miny);

            // 计算重心坐标实现
            for (int y = starty; y < endy; y++) {
                for (int x = startx; x < endx; x++) {
                    // 计算重心坐标
                    double area = (by - cy) * (ax - cx) + (cx - bx) * (ay - cy);
                    double w0 = ((by - cy) * (x - cx) + (cx - bx) * (y - cy)) / area;
                    double w1 = ((cy - ay) * (x - cx) + (ax - cx) * (y - cy)) / area;
                    double w2 = 1.0 - w0 - w1;

                    // 检查点是否在三角形内
                    if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                        // 插值深度
                        float z = w0 * tri.getVertex(0)[2] + w1 * tri.getVertex(1)[2] + w2 * tri.getVertex(2)[2];
                        
                        if (z < zBuffer[y * width + x]) {
                            zBuffer[y * width + x] = z;

                            // 插值纹理坐标和法线
                            double u = w0 * tri.uTex[0] + w1 * tri.uTex[1] + w2 * tri.uTex[2];
                            double v = w0 * tri.vTex[0] + w1 * tri.vTex[1] + w2 * tri.vTex[2];
                            
                            vec3 interpolatedNormal = w0 * tri.normal[0].value() + w1 * tri.normal[1].value() + w2 * tri.normal[2].value();
                            interpolatedNormal = interpolatedNormal.normalize();

                            // 获取纹理颜色或使用顶点颜色
                            vec3 baseColor = m.pTextureData ? m.getTexColor(u, v) : vec3(255, 255, 255);
                            
                            // 应用光照
                            vec3 color = lig(test.getVertex(0) * w0 + test.getVertex(1) * w1 + test.getVertex(2) * w2, 
                                            interpolatedNormal, baseColor, pCam ? pCam->pos : vec3(0, 0, 1));

                            int finalX = width - x - 1; // 翻转y坐标
                            frameBuffer[finalX * width + y] = (int(color[0]) << 16) | (int(color[1]) << 8) | int(color[2]);
                        }
                    }
                }
            }
        }
    }
}

GPURasterizer::~GPURasterizer()
{
}

void GPURasterizer::setRasterizeSize(int _width, int _height)
{
    width = _width;
    height = _height;
    resize = true;
    clearBuffer();
}

std::span<uint32_t> GPURasterizer::draw()
{
    renderScene();
    return std::span<uint32_t>(frameBuffer);
}

void GPURasterizer::addLight(vec3 pos)
{
    lig.lightPos.push_back(pos);
}

void GPURasterizer::pushModel(model &m)
{
    models.push_back(m);
}

void GPURasterizer::setBkColor(int r, int g, int b)
{
    bkColor = (r << 16) | (g << 8) | b;
}