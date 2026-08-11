#include <format>
#include <random>
#include "ogl/ogl.hpp"

#define PI 3.1415

struct TriangleLayer : Ogl::Layer
{
    Ogl::Texture Texture;

    TriangleLayer() : Texture(Ogl::ResolveTexture("test.png"))
    {
        Redraw = true;
    }

    void Draw() override
    {
        if (!Redraw)
            return;

        DrawTriangle(Ogl::Vec2(-0.75f, -0.75f), Ogl::Vec2(0.75f, -0.75f), Ogl::Vec2(0.0f, 0.75f), COLOR_TRANSPARENT, Texture);
    }
};

struct BallLayer : Ogl::Layer
{
    Ogl::Texture Texture;

    Ogl::Color BallColor;
    Ogl::Vec2 BallPos = Ogl::Vec2(0.0f);
    Ogl::Vec2 BallVelocity;
    float VelocityAngle;
    float TotalTime = 0.0f;

    Ogl::Color GradientColor1 = Ogl::Color(255, 0, 0, 128);
    Ogl::Color GradientColor2 = Ogl::Color(0, 0, 255, 128);
    const float GradientTime = 10.0f;

    const Ogl::Vec2 BallSize = Ogl::Vec2(0.5f);
    const float BallSpeed = 1.0f;
    const float TimeStep = 0.1f;
    
    BallLayer() : Ogl::Layer(true, GL_TRIANGLES, HEIGHT_MAX), Texture(Ogl::ResolveTexture("test.png"))
    {
        std::default_random_engine engine;
        engine.seed(std::time(nullptr));
        std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
        distribution.reset();

        VelocityAngle = 2.0f * PI * distribution(engine);
        BallVelocity = Ogl::Vec2::FromAngle(VelocityAngle) * BallSpeed;
    }

    float Clamp(float v, float min, float max)
    {
        v = v < min ? min : v;
        v = v > max ? max : v;
        return v;
    }

    Ogl::Color GetGradientColor(float t)
    {
        t = fmod(t / GradientTime, 1.0f);

        if (t <= TimeStep / GradientTime)
        {
            Ogl::Color oldColor = GradientColor1;
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

        Ogl::Vec2 bounds = Ogl::CameraSize / 2;
        if (BallPos.X + BallSize.X > bounds.X || BallPos.X < -bounds.X)
        {
            BallPos.X = Clamp(BallPos.X, -bounds.X, bounds.X);
            VelocityAngle = PI - VelocityAngle;
            BallVelocity = Ogl::Vec2::FromAngle(VelocityAngle) * BallSpeed;
        }
        if (BallPos.Y + BallSize.Y > bounds.Y || BallPos.Y < -bounds.Y)
        {
            BallPos.Y = Clamp(BallPos.Y, -bounds.Y, bounds.Y);
            VelocityAngle = -VelocityAngle;
            BallVelocity = Ogl::Vec2::FromAngle(VelocityAngle) * BallSpeed;
        }
    }
};

int main()
{
    Ogl::Initialize(300, 300, "Ball", false);
    Ogl::SetCameraSize(Ogl::Vec2(3.0f));

    TriangleLayer triangleLayer = {};
    Ogl::AddLayer(&triangleLayer);

    BallLayer ballLayer = {};
    Ogl::AddLayer(&ballLayer);

    Ogl::UpdateLoop();
    return 0;
}
