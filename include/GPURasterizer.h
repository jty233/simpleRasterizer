#pragma once

#include "Matrix.h"
#include "model.h"
#include "vec.h"
#include "camera.h"
#include "lightShader.h"
#include <vector>
#include <span>
#include <SDL.h>

// 包含OpenGL头文件
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

class GPURasterizer
{
private:
    int width, height;
    bool resize;
    uint32_t bkColor;
    camera *pCam = nullptr;
    lightShader lig;
    std::vector<std::reference_wrapper<model>> models;

    // OpenGL相关
    GLuint shaderProgram;
    GLuint VAO, VBO, EBO;
    GLuint framebuffer;
    GLuint colorBuffer;
    GLuint depthBuffer;
    
    // 存储顶点数据
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    void setupOpenGL();
    void setupBuffers();
    GLuint compileShader(const char* vertexSource, const char* fragmentSource);
    void updateUniforms();
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