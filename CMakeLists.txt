cmake_minimum_required(VERSION 3.10)
project(OGL VERSION 0.1.0 LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_compile_definitions(DEBUG_OUTPUT)

add_library(
    ogl STATIC 
    src/ogl.cpp
    src/buffer.cpp
    src/camera.cpp
    src/drawing.cpp
    src/input.cpp
    src/textures.cpp
    lib/glad/src/glad.c)
target_include_directories(
    ogl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ogl ${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw/glfw3.lib)

add_executable(test_ball tests/ball.cpp)
target_link_libraries(test_ball ogl)

add_executable(test_text tests/text.cpp)
target_link_libraries(test_text ogl)

add_executable(test_sierpinski tests/sierpinski.cpp)
target_link_libraries(test_sierpinski ogl)

add_executable(test_image_viewer tests/image_viewer.cpp)
target_link_libraries(test_image_viewer ogl)
