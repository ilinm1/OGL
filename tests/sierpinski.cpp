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
    std::vector<Vec2> Triangles = { Vec2(-0.5f) };
    int ColorIndex = 0;
    Color Pallete[6] = { Color(255, 255, 255), Color(255, 0, 0), Color(0, 255, 0), Color(0, 0, 255), Color(255, 255, 0), Color(255, 0, 255) };

    SierpinskiLayer() : Ogl::Layer()
    {
        IsWorldSpace = true;
    }

    Color GetColor()
    {
        ColorIndex = ++ColorIndex == sizeof(Pallete) / sizeof(Color) ? 0 : ColorIndex;
        return Pallete[ColorIndex];
    }

    void Draw() override
    {
        for (Vec2 triangle : Triangles)
        {
            DrawTriangle(triangle, Vec2(triangle.X + Size, triangle.Y), Vec2(triangle.X + Size / 2.0f, triangle.Y + Size * sqrtf(3) / 2.0f), GetColor());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(DelayMs));

        if (Iter++ == MaxIter)
        {
            Iter = 1;
            Size = 1.0f;
            Triangles = { Vec2(-0.5f) };
            return;
        }

        Size /= 2.0f;
        std::vector<Vec2> newTriangles;
        for (Vec2 triangle : Triangles)
        {
            newTriangles.push_back(Vec2(triangle.X + Size, triangle.Y)); //right triangle
            newTriangles.push_back(Vec2(triangle.X + Size / 2.0f, triangle.Y + Size * sqrtf(3) / 2.0f)); //upper triangle
        }
        Triangles.insert(Triangles.end(), newTriangles.begin(), newTriangles.end());
    }
};

int main()
{
    Ogl::Initialize(500, 500, "Sierpinski triangle", false);
    Ogl::SetCameraSize(Vec2(1.1f));

    SierpinskiLayer sierpinskiLayer = {};
    Ogl::AddLayer(&sierpinskiLayer);

    Ogl::UpdateLoop();
    return 0;
}
