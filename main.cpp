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
using namespace std;

const int width = 700, height = 700;
camera cam;
simpleWindow wnd;
rasterizer ras(width, height);
float getfps()
{
    static queue<int> q;
    int dur = 1;
    clock_t nt = clock();
    while (q.size() && (nt - q.front()) > dur * CLOCKS_PER_SEC)
        q.pop();
    q.push(nt);
    if (q.size() == 1)
        return 0;
    return q.size() / (float(nt - q.front()) / CLOCKS_PER_SEC);
}
int main()
{
    cout << std::thread::hardware_concurrency() << endl;
    wnd.create("hello world", width, height);
    wnd.setBkColor(220, 230, 210);

    cam.init(vec3(0, 0, 10), 45, (double)width / height, 0.1, 50);
    cam.setViewTarget(vec3(0, 0, 0));
    ras.setCamera(cam);

    ras.addLight(vec3(20, 20, 20));
    ras.addLight(vec3(-20, 20, 0));

    ras.bindSetPixelFunc([](int x, int y, int r, int g, int b)
                         { wnd.setPixel(x, y, r, g, b); });

    wnd.setResizeCallback(bind(&rasterizer::setRasterizeSize, &ras, placeholders::_1, placeholders::_2));

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
            t.setColor(148, 121, 92);
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
        if (key == 'A')
        {
            // cam.transform(vec3(-1, 0, 0));
            mod = mod.rotate(-5, vec3(0, 1, 0));
        }
        else if (key == 'D')
        {
            // cam.transform(vec3(1, 0, 0));
            mod = mod.rotate(5, vec3(0, 1, 0));
        }
        else if (key == 'W')
        {
            cam.transform(vec3(0, 0, -0.1));
        }
        else if (key == 'S')
        {
            cam.transform(vec3(0, 0, 0.1));
        }
        else if (key == 27)
            break;
        // mod.rotate(1, vec3(0.5, 0.7, 0.3));
        wnd.clear();
        ras.draw(); // 主要计算任务
        wnd.show();    // 消息处理循环

        // Sleep(10);
        // wnd.clear();
        cout << '\r' << getfps() << "      ";
    }
    return 0;
}