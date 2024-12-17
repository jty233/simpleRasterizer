cmake_minimum_required(VERSION 3.15.0)
project(games VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
find_package(OpenCV REQUIRED)
add_executable(games simpleWindow.cpp main.cpp Matrix.cpp Point.cpp Triangle.cpp rasterizer.cpp model.cpp camera.cpp vec.cpp
 lightShader.cpp)
target_link_libraries(games ${OpenCV_LIBS})
