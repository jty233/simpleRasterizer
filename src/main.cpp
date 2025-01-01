#include "simpleWindow.h"
#include "Matrix.h"
#include "Point.h"
#include "Triangle.h"
#include "vec.h"
#include "rasterizer.h"
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
rasterizer ras(width, height);
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
    ras.setBkColor(220, 230, 210);

    cam.init(vec3(0, 0, 10), 45, 0.1, 50);
    cam.setViewTarget(vec3(0, 0, 0));
    ras.setCamera(cam);

    ras.addLight(vec3(20, 20, 20));
    ras.addLight(vec3(-20, 20, 0));

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
    //     mod.getTriangle(i).setColor(0, 0, 100);

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

    ras.pushModel(mod);

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
        else if (key == 27)
            break;
        // mod.rotate(1, vec3(0.5, 0.7, 0.3));
        auto optResize = wnd.processWindowEvent();
        if (optResize)
            ras.setRasterizeSize(optResize->first, optResize->second);
        auto data = ras.draw(); // 主要计算任务
        wnd.show(data);    // 消息处理循环

        // this_thread::sleep_for(100ms);
        cout << '\r' << getfps() << "      ";
    }
    return 0;
}
