#include "simpleWindow.h"
#include "Matrix.h"
#include "Point.h"
#include "Triangle.h"
#include "vec.h"
#include "rasterizer.h"
#include "GPURasterizer.h"
#include "model.h"
#include "OBJ_Loader.h"
#include <iostream>
#include <random>
#include <time.h>
#include <cmath>
#include <any>
// #undef main
using namespace std;

const int width = 700, height = 700;
camera cam;
simpleWindow wnd;
rasterizer cpu_ras(width, height);  // CPU光栅化器
GPURasterizer gpu_ras(width, height);  // GPU光栅化器
bool useGPU = true;  // 默认使用GPU渲染
float getfps()
{  
    using fsec = chrono::time_point<std::chrono::_V2::system_clock, chrono::duration<double>>;
    static queue<fsec> q;
    int dur = 1;
    fsec curt = chrono::system_clock::now();
    while (q.size() && (curt - q.front()).count() > dur)
        q.pop();
    q.push(curt);
    if (q.size() == 1)
        return 0;
    return q.size() / (curt - q.front()).count();
}
int main(int argc,char* argv[])
{
    wnd.create("hello world", width, height);
    
    if (useGPU) {
        gpu_ras.setBkColor(220, 230, 210);
        cam.init(vec3(0, 0, 10), 45, 0.1, 50);
        cam.setViewTarget(vec3(0, 0, 0));
        gpu_ras.setCamera(cam);

        gpu_ras.addLight(vec3(20, 20, 20));
        gpu_ras.addLight(vec3(-20, 20, 0));
    } else {
        cpu_ras.setBkColor(220, 230, 210);
        cam.init(vec3(0, 0, 10), 45, 0.1, 50);
        cam.setViewTarget(vec3(0, 0, 0));
        cpu_ras.setCamera(cam);

        cpu_ras.addLight(vec3(20, 20, 20));
        cpu_ras.addLight(vec3(-20, 20, 0));
    }

    model mod;

    // mod = model::plain();
    // mod = mod.scale(vec3(2, 2, 2));
    // mod = mod.translate(vec3(-1, -1, 0));
    // for (int i = 0; i < 2; i++)
    //     mod.getTriangle(i).setColor(155, 0, 100);

    // mod = model::cube();
    // mod.translate(vec3(-1, -1, 0));
    // mod.scale(vec3(2, 2, 2));
    // for (int i = 0; i < 12; i++)
    //     mod.getTriangle(i).setColor(155, 0, 100);

    objl::Loader Loader;
    Loader.LoadFile("../models/spot/spot_triangulated_good.obj");
    for (auto mesh : Loader.LoadedMeshes)
    {
        for (int i = 0; i < mesh.Vertices.size(); i += 3)
        {
            Triangle t;
            t.setColor(255, 255, 255);
            for (int j = 0; j < 3; j++)
            {
                t.setVertex(vec3(mesh.Vertices[i + j].Position.X, mesh.Vertices[i + j].Position.Y, mesh.Vertices[i + j].Position.Z), j);
                t.setNormal(vec3(mesh.Vertices[i + j].Normal.X, mesh.Vertices[i + j].Normal.Y, mesh.Vertices[i + j].Normal.Z), j);
                t.setTexCoord(mesh.Vertices[i + j].TextureCoordinate.X, mesh.Vertices[i + j].TextureCoordinate.Y, j);
            }
            mod.addTriangle(t);
        }
    }
    mod.loadTexture("../models/spot/spot_texture.png");
    mod = mod.scale(vec3(2.5, 2.5, 2.5));
    mod = mod.rotate(140, vec3(0, 1, 0));

    if (useGPU) {
        gpu_ras.pushModel(mod);
    } else {
        cpu_ras.pushModel(mod);
    }

    while (!wnd.shouldClose())
    {
        char key = wnd.getKey();

        // if (key == 'A' || key == 'D')
        // {
        //     double deg = key == 'A' ? 1 : -1;
        //     mod.rotate(deg, vec3(1, 0, 0));
        // }
        if (key == 'a')
        {
            // cam.transform(vec3(-1, 0, 0));
            // mod = mod.rotate(-5, vec3(0, 1, 0));
            mod = mod.translate({1, 0, 0});
        }
        else if (key == 'd')
        {
            mod = mod.translate({-1, 0, 0});
            // cam.transform(vec3(1, 0, 0));
            // mod = mod.rotate(5, vec3(0, 1, 0));
        }
        else if (key == 'w')
        {
            cam.transform(vec3(0, 0, -0.1));
        }
        else if (key == 's')
        {
            cam.transform(vec3(0, 0, 0.1));
        }
        else if (key == 'g')  // 切换GPU/CPU渲染
        {
            useGPU = !useGPU;
            cout << "Switched to " << (useGPU ? "GPU" : "CPU") << " rendering" << endl;
        }
        else if (key == 27)
            break;
        // mod.rotate(1, vec3(0.5, 0.7, 0.3));
        auto optResize = wnd.processWindowEvent();
        if (optResize)
        {
            if (useGPU) {
                gpu_ras.setRasterizeSize(optResize->first, optResize->second);
            } else {
                cpu_ras.setRasterizeSize(optResize->first, optResize->second);
            }
        }
        
        std::span<uint32_t> data;
        if (useGPU) {
            data = gpu_ras.draw(); // GPU渲染任务
        } else {
            data = cpu_ras.draw(); // CPU渲染任务
        }
        
        wnd.show(data);    // 消息处理循环

        // this_thread::sleep_for(100ms);
        cout << '\r' << getfps() << "      ";
    }
    return 0;
}
