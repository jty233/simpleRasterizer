#include "GPURasterizer.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <span>

// 顶点着色器源码
static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 ViewPos;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    ViewPos = viewPos;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

// 片段着色器源码
static const char* fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 ViewPos;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform sampler2D texture_diffuse1;
uniform bool hasTexture;

void main()
{
    // 环境光
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // 漫反射
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // 镜面反射
    float specularStrength = 0.5;
    vec3 viewDir = normalize(ViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result;
    if (hasTexture) {
        vec3 textureColor = texture(texture_diffuse1, TexCoord).rgb;
        result = (ambient + diffuse + specular) * textureColor;
    } else {
        result = ambient + diffuse + specular;
    }
    
    FragColor = vec4(result, 1.0);
}
)";

void GPURasterizer::setupOpenGL()
{
    // OpenGL上下文应该由窗口管理，我们不需要在这里创建
    // 编译着色器
    shaderProgram = compileShader(vertexShaderSource, fragmentShaderSource);

    // 设置帧缓冲区
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // 创建颜色缓冲区纹理
    glGenTextures(1, &colorBuffer);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);

    // 创建深度缓冲区渲染缓冲区
    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

    // 检查帧缓冲区是否完整
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 启用深度测试
    glEnable(GL_DEPTH_TEST);
}

void GPURasterizer::setupBuffers()
{
    // 生成VAO, VBO, EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // 顶点位置
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法线
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 纹理坐标
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

GLuint GPURasterizer::compileShader(const char* vertexSource, const char* fragmentSource)
{
    // 编译顶点着色器
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // 检查编译错误
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
    }

    // 编译片段着色器
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // 检查编译错误
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
    }

    // 创建着色器程序
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // 检查链接错误
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }

    // 删除着色器对象
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void GPURasterizer::updateUniforms()
{
    glUseProgram(shaderProgram);

    // 设置变换矩阵
    Matrix modelMatrix = models[0].get().modelMatrix;
    Matrix viewMatrix = pCam ? pCam->viewMatrix : Matrix::identity(4);
    Matrix projMatrix = pCam ? pCam->projectionMatrix : Matrix::identity(4);

    // 传递到着色器
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    GLint hasTextureLoc = glGetUniformLocation(shaderProgram, "hasTexture");

    // 将Matrix转换为OpenGL兼容格式（列主序）
    float modelMat[16], viewMat[16], projMat[16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            modelMat[j * 4 + i] = modelMatrix[i][j];
            viewMat[j * 4 + i] = viewMatrix[i][j];
            projMat[j * 4 + i] = projMatrix[i][j];
        }
    }

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, modelMat);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMat);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMat);
    
    if (pCam) {
        glUniform3f(viewPosLoc, pCam->pos[0], pCam->pos[1], pCam->pos[2]);
    }
    
    if (!lig.lightPos.empty()) {
        vec3 lightPos = lig.lightPos[0];
        glUniform3f(lightPosLoc, lightPos[0], lightPos[1], lightPos[2]);
        glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f); // 白光
    }
    
    // 检查是否有纹理
    bool hasTexture = !models.empty() && models[0].get().pTextureData != nullptr;
    glUniform1i(hasTextureLoc, hasTexture);
}

void GPURasterizer::renderScene()
{
    // 收集所有模型的顶点数据
    vertices.clear();
    indices.clear();
    
    int vertexOffset = 0;
    for (auto mref : models) {
        model &m = mref.get();
        
        // 遍历模型中的所有三角形
        for (Triangle tri : m.tris) {
            // 将三角形的三个顶点添加到顶点数组
            for (int i = 0; i < 3; i++) {
                vec3 vertex = tri.getVertex(i);
                vec3 normal = tri.normal[i] ? tri.normal[i].value() : vec3(0, 0, 0);
                
                vertices.push_back(vertex[0]);  // x
                vertices.push_back(vertex[1]);  // y
                vertices.push_back(vertex[2]);  // z
                vertices.push_back(normal[0]);  // nx
                vertices.push_back(normal[1]);  // ny
                vertices.push_back(normal[2]);  // nz
                vertices.push_back(tri.uTex[i]); // u
                vertices.push_back(tri.vTex[i]); // v
            }
            
            // 添加索引（每个三角形3个索引）
            indices.push_back(vertexOffset + 0);
            indices.push_back(vertexOffset + 1);
            indices.push_back(vertexOffset + 2);
            vertexOffset += 3;
        }
    }
    
    // 更新缓冲区
    setupBuffers();
    
    // 绑定帧缓冲区
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, width, height);

    // 清除缓冲区
    float r = ((bkColor >> 16) & 0xFF) / 255.0f;
    float g = ((bkColor >> 8) & 0xFF) / 255.0f;
    float b = (bkColor & 0xFF) / 255.0f;
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!models.empty()) {
        updateUniforms();

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

GPURasterizer::GPURasterizer(int width, int height) : width(width), height(height), resize(true)
{
    setupOpenGL();
}

GPURasterizer::~GPURasterizer()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &colorBuffer);
    glDeleteRenderbuffers(1, &depthBuffer);
}

void GPURasterizer::setRasterizeSize(int _width, int _height)
{
    width = _width;
    height = _height;
    resize = true;
    
    // 重新配置帧缓冲区
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
}

std::span<uint32_t> GPURasterizer::draw()
{
    // 执行渲染到帧缓冲区
    renderScene();
    
    // 从帧缓冲区读取颜色数据
    static std::vector<uint32_t> frameBuffer;
    frameBuffer.resize(width * height);
    
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, frameBuffer.data());
    
    // 翻转Y轴，因为OpenGL的坐标系与图像坐标系相反
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width; x++) {
            int topIdx = y * width + x;
            int bottomIdx = (height - 1 - y) * width + x;
            // 交换RGB值，因为GL读取是RGB格式，但我们需要RGB888格式
            uint32_t topPixel = frameBuffer[topIdx];
            uint32_t bottomPixel = frameBuffer[bottomIdx];
            
            // 将RGB转换为RGB888格式
            uint8_t r1 = (topPixel >> 0) & 0xFF;
            uint8_t g1 = (topPixel >> 8) & 0xFF;
            uint8_t b1 = (topPixel >> 16) & 0xFF;
            frameBuffer[topIdx] = (r1 << 16) | (g1 << 8) | b1;
            
            uint8_t r2 = (bottomPixel >> 0) & 0xFF;
            uint8_t g2 = (bottomPixel >> 8) & 0xFF;
            uint8_t b2 = (bottomPixel >> 16) & 0xFF;
            frameBuffer[bottomIdx] = (r2 << 16) | (g2 << 8) | b2;
            
            std::swap(frameBuffer[topIdx], frameBuffer[bottomIdx]);
        }
    }
    
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