#include "rasterizer.h"
#include <cmath>
#include <algorithm>
using namespace std;

void rasterizer::clearBuffer()
{
    int sz = width * height;
    zBuffer.resize(sz);
    fill_n(zBuffer.begin(), sz, numeric_limits<float>::infinity());
    frameBuffer.assign(sz, bkColor);
}
static void computeBarycentric2D(double x, double y, const Triangle &t, double *param)
{
    double xa = t.getVertex(0).data[0], ya = t.getVertex(0).data[1];
    double xb = t.getVertex(1).data[0], yb = t.getVertex(1).data[1];
    double xc = t.getVertex(2).data[0], yc = t.getVertex(2).data[1];
    param[0] = (x * (yb - yc) + (xc - xb) * y + xb * yc - xc * yb) / (xa * (yb - yc) + (xc - xb) * ya + xb * yc - xc * yb);
    param[1] = (x * (yc - ya) + (xa - xc) * y + xc * ya - xa * yc) / (xb * (yc - ya) + (xa - xc) * yb + xc * ya - xa * yc);
    param[2] = (x * (ya - yb) + (xb - xa) * y + xa * yb - xb * ya) / (xc * (ya - yb) + (xb - xa) * yc + xa * yb - xb * ya);
}
double calculateIntersection(double c, const Point &p1, const Point &p2)
{
    // 计算斜率
    double dx = p2.data[0] - p1.data[0];
    double dy = p2.data[1] - p1.data[1];
    double m = dy / dx; // 斜率

    // 计算交点的 y 坐标
    double y = p1.data[1] + m * (c - p1.data[0]);
    return y;
}

// 检查边是否与直线 x = c 相交
void checkEdgeForIntersections(double c, const Point &p1, const Point &p2, vector<double> &intersections)
{
    if (p1.data[0] == p2.data[0])
    {
        // 垂直边
        if (p1.data[0] == c)
        {
            // 整条边都在 x = c 上，添加两个端点
            intersections.push_back(p1.data[1]);
            intersections.push_back(p2.data[1]);
        }
        // 否则不相交
    }
    else
    {
        // 非垂直边，检查是否与 x = c 相交
        if ((p1.data[0] - c) * (p2.data[0] - c) <= 0)
        {
            // 计算交点并添加
            double intersection = calculateIntersection(c, p1, p2);
            intersections.push_back(intersection);
        }
    }
}

// 找到直线 x = c 与三角形边界的所有交点
optional<pair<int, int>> findIntersections(double c, const Point &A, const Point &B, const Point &C)
{
    vector<double> intersections;

    // 检查边 AB
    checkEdgeForIntersections(c, A, B, intersections);

    // 检查边 BC
    checkEdgeForIntersections(c, B, C, intersections);

    // 检查边 CA
    checkEdgeForIntersections(c, C, A, intersections);

    if (intersections.empty())
        return nullopt;

    sort(intersections.begin(), intersections.end());

    return make_optional<pair<int, int>>(ceil(intersections.front()), intersections.back());
}

void rasterizer::drawTriangle(Triangle tri, Triangle ctri, const model &mod, int startX, int endX, bool mutiThread)
{

    for (int i = startX; i < endX; i++)
    {
        auto opt = findIntersections(i, tri.getVertex(0), tri.getVertex(1), tri.getVertex(2));
        if (opt)
        {
            auto [p1, p2] = *opt;
            p1 = max(0, p1);
            p2 = min(height - 1, p2);
            if (mutiThread)
                threads.push_back(poolIns.assign(bind(&rasterizer::rasterizeLine, this, tri, ctri, ref(mod), i, p1, p2)));
            else
                rasterizeLine(tri, ctri, mod, i, p1, p2);
        }
    }
}

void rasterizer::rasterizeLine(Triangle tri, Triangle ctri, const model &mod, int x, int startY, int endY)
{
    for (int j = startY; j <= endY; j++)
    {
        double param[3];
        computeBarycentric2D(x, j, tri, param);
        vec3 p, p2;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
            {
                p[i] += param[j] * tri.getVertex(j)[i];
                p2[i] += param[j] * ctri.getVertex(j)[i];
            }
        float z = float(-p[2]);
        if (z >= zBuffer[j * width + x])
            continue;

        double r = 0, g = 0, b = 0;
        for (int i = 0; i < 3; i++)
        {
            r += param[i] * tri.colorR[i];
            g += param[i] * tri.colorG[i];
            b += param[i] * tri.colorB[i];
        }

        vec3 nor;
        double uTex = 0, vTex = 0;
        for (int i = 0; i < 3; i++)
        {
            nor += tri.normal[i].value() * param[i];
            uTex += tri.uTex[i] * param[i];
            vTex += tri.vTex[i] * param[i];
        }
        // nor.abs();

        // vec3 color = texColor;
        vec3 color;
        vec3 baseColor = mod.pTextureData ? mod.getTexColor(uTex, vTex) : vec3(r, g, b);
        if (lig.lightPos.empty())
            color = baseColor;
        else
            color = lig(p2, nor, baseColor, pCam ? pCam->pos : vec3(0, 0, 1));
        // vec3 color = lig(p2, nor, vec3(r, g, b), pCam->pos);
        // vec3 color = (nor + vec3(1, 1, 1)) * 255 / 2.0;
        float oldValue = zBuffer[j * width + x].load();
        while (z < oldValue)
        {
            if (zBuffer[j * width + x].compare_exchange_weak(oldValue, z))
            {
                setPixel(j, x, color[0], color[1], color[2]);
                break;
            }
        }
    }
}

void rasterizer::setPixel(int x, int y, int r, int g, int b)
{
    x = height - x - 1; // 翻转y坐标
    frameBuffer[x * width + y] = (r << 16) | (g << 8) | b;
}

rasterizer::rasterizer(int width, int height) : width(width), height(height), resize(true), poolIns(ThreadPool::getInstance())
{
}
void rasterizer::setRasterizeSize(int _width, int _height)
{
    width = _width;
    height = _height;
    resize = true;
}

std::span<uint32_t> rasterizer::draw()
{
    static future<void> cls;
    static Matrix viewpointMatrix;
    static Matrix vpv;
    if (pCam && resize)
    {
        pCam->resize(width, height);
        resize = false;
        cls = async(&rasterizer::clearBuffer, this);

        viewpointMatrix = {
            {width / 2., 0, 0, width / 2.},
            {0, height / 2., 0, height / 2.},
            {0, 0, 1, 0},
            {0, 0, 0, 1}};
        
    }
    if (pCam)
        vpv = viewpointMatrix * pCam->projectionMatrix * pCam->viewMatrix;

    if (cls.valid())
        cls.get();
    // const vec3 &view_pos = pCam->pos;

    for (auto mref : models)
    {
        model &m = mref.get();
        Matrix mvpv, mv;
        if (pCam)
        {
            mvpv = vpv * m.modelMatrix;
            mv = pCam->viewMatrix * m.modelMatrix;
        }
        else
        {
            mvpv = viewpointMatrix * m.modelMatrix;
            mv = m.modelMatrix;
        }
        // clearBuffer();
        for (auto p : m.lines)
        {
            Triangle tri = {p.first, p.second, Point{}};
            tri = (mvpv * tri).normalize();
            drawLine(tri.getVertex(0), tri.getVertex(1));
        }

        for (Triangle ctri : m.tris)
        {
            Triangle tri = (mvpv * ctri).normalize();
            Triangle test = mvpv * ctri;
            ctri = (mv * ctri).normalize();

            vec3 ab(ctri.getVertex(1)[0] - ctri.getVertex(0)[0], ctri.getVertex(1)[1] - ctri.getVertex(0)[1], ctri.getVertex(1)[2] - ctri.getVertex(0)[2]);
            vec3 ac(ctri.getVertex(2)[0] - ctri.getVertex(0)[0], ctri.getVertex(2)[1] - ctri.getVertex(0)[1], ctri.getVertex(2)[2] - ctri.getVertex(0)[2]);
            vec3 nor = (ab.cross(ac)).normalize();
            for (int i = 0; i < 3; i++)
            {
                if (tri.normal[i])
                    tri.normal[i] = (mv * tri.normal[i].value()).normalize();
                else
                    tri.normal[i] = nor;
            }
            double ax = tri.getVertex(0)[0], ay = tri.getVertex(0)[1];
            double bx = tri.getVertex(1)[0], by = tri.getVertex(1)[1];
            double cx = tri.getVertex(2)[0], cy = tri.getVertex(2)[1];

            int minx = min({ax, bx, cx});
            int maxx = max({ax, bx, cx});
            int miny = min({ay, by, cy});
            int maxy = max({ay, by, cy});

            int endx = min(width, maxx + 1);
            int startx = max(0, minx);
            int endy = min(height, maxy + 1);
            int starty = max(0, miny);
            bool interThread = (endy - starty > 100);
            if (interThread)
                drawTriangle(tri, ctri, m, startx, endx, true);
            else
                threads.push_back(poolIns.assign(bind(&rasterizer::drawTriangle, this, tri, ctri, ref(m), startx, endx, false)));
        }
    }
    for (auto &f : threads)
        f.get();
    threads.clear();
    cls = async(&rasterizer::clearBuffer, this);
    return span<uint32_t>(frameBuffer);
}

void rasterizer::addLight(vec3 pos)
{
    lig.lightPos.push_back(pos);
}

void rasterizer::drawLine(Point begin, Point end, vec3 lineColor)
{
    auto x1 = begin[0];
    auto y1 = begin[1];
    auto x2 = end[0];
    auto y2 = end[1];

    int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;
    double lineLen = 0;
    for (int i = 0; i < 2; i++)
        lineLen += pow(begin[i] - end[i], 2);
    lineLen = sqrt(lineLen);

    dx = int(x2 - x1);
    dy = int(y2 - y1);
    dx1 = int(fabs(dx));
    dy1 = int(fabs(dy));
    px = 2 * dy1 - dx1;
    py = 2 * dx1 - dy1;

    auto drawPixel = [&]()
    {
        // wnd.setPixel(x, y, lineColor);
        double curLen = pow(x - begin[0], 2) + pow(y - begin[1], 2);
        curLen = sqrt(curLen);
        double klen = curLen / lineLen;
        float z = -float((1 - klen) * begin[2] + klen * end[2]);
        z -= 0.0001f;
        if (z <= zBuffer[y * width + x])
        {
            setPixel(x, y, int(lineColor[0]), int(lineColor[1]), int(lineColor[2]));
            zBuffer[y * width + x] = z;
        }
    };

    if (dy1 <= dx1)
    {
        if (dx >= 0)
        {
            x = (int)x1;
            y = (int)y1;
            xe = (int)x2;
        }
        else
        {
            x = (int)x2;
            y = (int)y2;
            xe = (int)x1;
        }
        drawPixel();
        for (i = 0; x < xe; i++)
        {
            x = x + 1;
            if (px < 0)
            {
                px = px + 2 * dy1;
            }
            else
            {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
                {
                    y = y + 1;
                }
                else
                {
                    y = y - 1;
                }
                px = px + 2 * (dy1 - dx1);
            }
            drawPixel();
        }
    }
    else
    {
        if (dy >= 0)
        {
            x = (int)x1;
            y = (int)y1;
            ye = (int)y2;
        }
        else
        {
            x = (int)x2;
            y = (int)y2;
            ye = (int)y1;
        }
        drawPixel();
        for (i = 0; y < ye; i++)
        {
            y = y + 1;
            if (py <= 0)
            {
                py = py + 2 * dx1;
            }
            else
            {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
                {
                    x = x + 1;
                }
                else
                {
                    x = x - 1;
                }
                py = py + 2 * (dx1 - dy1);
            }
            drawPixel();
        }
    }
}

void rasterizer::setBkColor(int r, int g, int b)
{
    bkColor = (r << 16) | (g << 8) | b;
}
