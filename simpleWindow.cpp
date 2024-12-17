#include "simpleWindow.h"
#include <string>
#include <iostream>
#include <thread>
#include <future>

using namespace std;
using namespace cv;

void simpleWindow::create(const char *name, int width, int height)
{
    wndMat.create(height, width, CV_8UC3);
    lstWidth = nWidth = width;
    lstHeight = nHeight = height;

    windowName = name;
    namedWindow(windowName, cv::WINDOW_NORMAL);
    resizeWindow(windowName, width, height);

    show();
}

void simpleWindow::show()
{
    imshow(windowName, wndMat);
    resize(wndMat, wndMat, {lstWidth, lstHeight});
    auto res = getWindowImageRect(windowName);
    if (resizeCallback)
        resizeCallback(res.width, res.height);
    lstWidth = res.width;
    lstHeight = res.height;
    lastPress = waitKey(1);
}

bool simpleWindow::shouldClose()
{
    return getWindowProperty(windowName, WND_PROP_VISIBLE) < 1;
}

void simpleWindow::setPixel(int x, int y, int r, int g, int b)
{
    y = wndMat.rows - y;
    wndMat.at<Vec3b>(y, x)[0] = b;
    wndMat.at<Vec3b>(y, x)[1] = g;
    wndMat.at<Vec3b>(y, x)[2] = r;
}

void simpleWindow::setPixel(int x, int y, vec3 color)
{
    setPixel(x, y, (int)color[0], (int)color[1], (int)color[2]);
}

void simpleWindow::clear()
{
    for (int i = 0; i < wndMat.rows; i++)
        for (int j = 0; j < wndMat.cols; j++)
            wndMat.at<Vec3b>(i, j) = Vec3b{(uchar)bkColor[0], (uchar)bkColor[1], (uchar)bkColor[2]};
}

void simpleWindow::setBkColor(int r, int g, int b)
{
    bkColor = vec3(r, g, b);
    clear();
}

bool simpleWindow::press(char key)
{
    return bPress[key];
}

char simpleWindow::getKey()
{
    char key = lastPress;
    lastPress = 0;
    if (key & 32)
        key ^= 32;
    return key;
}
