cmake_minimum_required(VERSION 3.10)
project(OGL VERSION 0.1.0 LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_compile_definitions(DEBUG_OUTPUT)

include_directories(lib/glfw/include lib/glad/include lib/stb src)
add_library(ogl STATIC 
    src/ogl.cpp
    src/buffer.cpp
    src/camera.cpp
    src/drawing.cpp
    src/input.cpp
    src/textures.cpp
    lib/glad/src/glad.c)
target_link_libraries(ogl ${PROJECT_SOURCE_DIR}/lib/glfw/glfw3.lib)

add_executable(test_ball tests/ball.cpp)
target_link_libraries(test_ball ogl)

add_executable(test_text tests/text.cpp)
target_link_libraries(test_text ogl)
