cmake_minimum_required(VERSION 3.15.0)
project(games VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D__DEBUG__)
endif()

find_package(SDL2 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(include ${CMAKE_SOURCE_DIR})
file(GLOB SOURCE src/*.cpp)
add_executable(games ${SOURCE})
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    ${SDL2_LIBRARIES}
    SDL2::SDL2main
    SDL2::SDL2
    OpenGL::GL
    ${CMAKE_DL_LIBS}
)
