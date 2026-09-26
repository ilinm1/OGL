#include <chrono>
#include <thread>
#include <iostream>
#include "tc/graphics.hpp"

namespace Tcg = Tc::Graphics;

struct SierpinskiLayer : Tcg::Layer
{
    const int MaxIter = 10;
    const float InitialTriangleSize = 1.8f;
    const Tc::Vec2 InitialTrianglePosition = Tc::Vec2(-0.9f);
    const int DelayMs = 500;

    int Iter = 1;
    float Size = InitialTriangleSize;
    std::vector<Tc::Vec2> Triangles = { InitialTrianglePosition };
    int ColorIndex = 0;
    Tc::Color Pallete[6] = { Tc::Color(255, 255, 255), Tc::Color(255, 0, 0), Tc::Color(0, 255, 0), Tc::Color(0, 0, 255), Tc::Color(255, 255, 0), Tc::Color(255, 0, 255) };

    SierpinskiLayer() {}

    Tc::Color GetColor()
    {
        ColorIndex = ++ColorIndex == sizeof(Pallete) / sizeof(Tc::Color) ? 0 : ColorIndex;
        return Pallete[ColorIndex];
    }

    void Draw() override
    {
        for (Tc::Vec2 triangle : Triangles)
        {
            DrawTriangle(triangle, Tc::Vec2(triangle.X + Size, triangle.Y), Tc::Vec2(triangle.X + Size / 2.0f, triangle.Y + Size * sqrtf(3) / 2.0f), GetColor());
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
        std::vector<Tc::Vec2> newTriangles;
        for (Tc::Vec2 triangle : Triangles)
        {
            newTriangles.push_back(Tc::Vec2(triangle.X + Size, triangle.Y)); //right triangle
            newTriangles.push_back(Tc::Vec2(triangle.X + Size / 2.0f, triangle.Y + Size * sqrtf(3) / 2.0f)); //upper triangle
        }
        Triangles.insert(Triangles.end(), newTriangles.begin(), newTriangles.end());
    }
};

int main()
{
    Tcg::Initialize(500, 500, "Sierpinski triangle");

    SierpinskiLayer sierpinskiLayer = {};
    Tcg::AddLayer(&sierpinskiLayer);

    Tcg::UpdateLoop();
    return 0;
}
