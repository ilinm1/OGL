#include <chrono>
#include <thread>
#include <iostream>
#include "ogl/ogl.hpp"

struct SierpinskiLayer : Ogl::Layer
{
    const int MaxIter = 10;
    const float InitialTriangleSize = 1.8f;
    const Ogl::Vec2 InitialTrianglePosition = Ogl::Vec2(-0.9f);
    const int DelayMs = 500;

    int Iter = 1;
    float Size = InitialTriangleSize;
    std::vector<Ogl::Vec2> Triangles = { InitialTrianglePosition };
    int ColorIndex = 0;
    Ogl::Color Pallete[6] = { Ogl::Color(255, 255, 255), Ogl::Color(255, 0, 0), Ogl::Color(0, 255, 0), Ogl::Color(0, 0, 255), Ogl::Color(255, 255, 0), Ogl::Color(255, 0, 255) };

    SierpinskiLayer() {}

    Ogl::Color GetColor()
    {
        ColorIndex = ++ColorIndex == sizeof(Pallete) / sizeof(Ogl::Color) ? 0 : ColorIndex;
        return Pallete[ColorIndex];
    }

    void Draw() override
    {
        for (Ogl::Vec2 triangle : Triangles)
        {
            DrawTriangle(triangle, Ogl::Vec2(triangle.X + Size, triangle.Y), Ogl::Vec2(triangle.X + Size / 2.0f, triangle.Y + Size * sqrtf(3) / 2.0f), GetColor());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(DelayMs));

        if (Iter++ == MaxIter)
        {
            Iter = 1;
            Size = InitialTriangleSize;
            Triangles = { InitialTrianglePosition };
            return;
        }

        Size /= 2.0f;
        std::vector<Ogl::Vec2> newTriangles;
        for (Ogl::Vec2 triangle : Triangles)
        {
            newTriangles.push_back(Ogl::Vec2(triangle.X + Size, triangle.Y)); //right triangle
            newTriangles.push_back(Ogl::Vec2(triangle.X + Size / 2.0f, triangle.Y + Size * sqrtf(3) / 2.0f)); //upper triangle
        }
        Triangles.insert(Triangles.end(), newTriangles.begin(), newTriangles.end());
    }
};

int main()
{
    Ogl::Initialize(500, 500, "Sierpinski triangle", false);

    SierpinskiLayer sierpinskiLayer = {};
    Ogl::AddLayer(&sierpinskiLayer);

    Ogl::UpdateLoop();
    return 0;
}
