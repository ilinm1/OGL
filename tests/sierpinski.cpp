#include <chrono>
#include <thread>
#include <iostream>
#include <ogl.hpp>

struct SierpinskiLayer : Ogl::Layer
{
    int Iter = 1;
    const int MaxIter = 10;
    float Size = 1.0f;
    const int DelayMs = 500;
    std::vector<Ogl::Vec2> Triangles = { Ogl::Vec2(-0.5f) };
    int ColorIndex = 0;
    Ogl::Color Pallete[6] = { Ogl::Color(255, 255, 255), Ogl::Color(255, 0, 0), Ogl::Color(0, 255, 0), Ogl::Color(0, 0, 255), Ogl::Color(255, 255, 0), Ogl::Color(255, 0, 255) };

    SierpinskiLayer() : Ogl::Layer()
    {
        IsWorldSpace = true;
    }

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
            Size = 1.0f;
            Triangles = { Ogl::Vec2(-0.5f) };
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
    Ogl::SetCameraSize(Ogl::Vec2(1.1f));

    SierpinskiLayer sierpinskiLayer = {};
    Ogl::AddLayer(&sierpinskiLayer);

    Ogl::UpdateLoop();
    return 0;
}
