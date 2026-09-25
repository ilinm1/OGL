#include <format>
#include <random>
#include "tc/graphics.hpp"

namespace Tcg = Tc::Graphics;

#define PI 3.1415

struct TriangleLayer : Tcg::Layer
{
    Tcg::Texture Texture;

    TriangleLayer() : Texture(Tcg::ResolveTexture("test.png"))
    {
        Redraw = true;
    }

    void Draw() override
    {
        if (!Redraw)
            return;

        DrawTriangle(Tc::Vec2(-0.75f, -0.75f), Tc::Vec2(0.75f, -0.75f), Tc::Vec2(0.0f, 0.75f), COLOR_TRANSPARENT, Texture);
    }
};

struct BallLayer : Tcg::Layer
{
    Tcg::Texture Texture;

    Tc::Color BallColor;
    Tc::Vec2 BallPos = Tc::Vec2(0.0f);
    Tc::Vec2 BallVelocity;
    float VelocityAngle;
    float TotalTime = 0.0f;

    Tc::Color GradientColor1 = Tc::Color(255, 0, 0, 128);
    Tc::Color GradientColor2 = Tc::Color(0, 0, 255, 128);
    const float GradientTime = 10.0f;

    const Tc::Vec2 BallSize = Tc::Vec2(0.5f);
    const float BallSpeed = 1.0f;
    const float TimeStep = 0.1f;
    
    BallLayer() : Tcg::Layer(true, GL_TRIANGLES, HEIGHT_MAX), Texture(Tcg::ResolveTexture("test.png"))
    {
        std::default_random_engine engine;
        engine.seed(std::time(nullptr));
        std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
        distribution.reset();

        VelocityAngle = 2.0f * PI * distribution(engine);
        BallVelocity = Tc::Vec2::FromAngle(VelocityAngle) * BallSpeed;
    }

    float Clamp(float v, float min, float max)
    {
        v = v < min ? min : v;
        v = v > max ? max : v;
        return v;
    }

    Tc::Color GetGradientColor(float t)
    {
        t = fmod(t / GradientTime, 1.0f);

        if (t <= TimeStep / GradientTime)
        {
            Tc::Color oldColor = GradientColor1;
            GradientColor1 = GradientColor2;
            GradientColor2 = oldColor;
        }

        return GradientColor1 * (1.0f - t) + GradientColor2 * t;
    }

    void Draw() override
    {
        TotalTime += TimeStep;
        BallPos += BallVelocity * TimeStep;
        BallColor = GetGradientColor(TotalTime);
        DrawRect(BallPos, BallPos + BallSize, BallColor, Texture);

        Tc::Vec2 bounds = Tcg::CameraSize / 2;
        if (BallPos.X + BallSize.X > bounds.X || BallPos.X < -bounds.X)
        {
            BallPos.X = Clamp(BallPos.X, -bounds.X, bounds.X);
            VelocityAngle = PI - VelocityAngle;
            BallVelocity = Tc::Vec2::FromAngle(VelocityAngle) * BallSpeed;
        }
        if (BallPos.Y + BallSize.Y > bounds.Y || BallPos.Y < -bounds.Y)
        {
            BallPos.Y = Clamp(BallPos.Y, -bounds.Y, bounds.Y);
            VelocityAngle = -VelocityAngle;
            BallVelocity = Tc::Vec2::FromAngle(VelocityAngle) * BallSpeed;
        }
    }
};

int main()
{
    Tcg::Initialize(300, 300, "Ball");
    Tcg::SetCameraSize(Tc::Vec2(3.0f));

    TriangleLayer triangleLayer = {};
    Tcg::AddLayer(&triangleLayer);

    BallLayer ballLayer = {};
    Tcg::AddLayer(&ballLayer);

    Tcg::UpdateLoop();
    return 0;
}
