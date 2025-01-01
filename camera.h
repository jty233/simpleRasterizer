#pragma once
#include "vec.h"
#include "Matrix.h"
#include <optional>
class camera
{
    friend class rasterizer;

private:
    double fov, aspect_ratio, zNear, zFar, top;
    vec3 pos;
    Matrix projectionMatrix;
    Matrix viewMatrix;
    vec3 lookat,right,up;
    const vec3 worldUp = vec3(0, 1, 0);
    std::optional<vec3> viewTarget;

    void updViewMartx();
public:
    camera() {}
    void init(vec3 _pos, double _fov, double _zNear, double _zFar, vec3 _lookat = {0, 0, -1});
    void resize(int width, int height);
    void transform(vec3 v);
    void setViewTarget(vec3 target);
};
