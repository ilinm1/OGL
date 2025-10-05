#include <format>
#include <random>
#include "../include/ogl.hpp"

#define PI 3.1415

struct TriangleLayer : Ogl::Layer
{
    Ogl::Texture Texture;

    TriangleLayer() : Ogl::Layer()
    {
        Texture = Ogl::ResolveTexture("test.png");
        Redraw = true;
    }

    void Draw() override
    {
        if (!Redraw)
            return;

        DrawTriangle(Vec2(-0.75f, -0.75f), Vec2(0.75f, -0.75f), Vec2(0.0f, 0.75f), COLOR_TRANSPARENT, Texture);
    }
};

struct BallLayer : Ogl::Layer
{
    Ogl::Texture Texture;

    Color BallColor;
    Vec2 BallPos = Vec2(0.0f);
    Vec2 BallVelocity;
    float VelocityAngle;
    float TotalTime = 0.0f;

    Color GradientColor1 = Color(255, 0, 0, 128);
    Color GradientColor2 = Color(0, 0, 255, 128);
    const float GradientTime = 10.0f;

    const Vec2 BallSize = Vec2(0.5f);
    const float BallSpeed = 1.0f;
    const float TimeStep = 0.1f;
    
    BallLayer() : Ogl::Layer()
    {
        DrawingDepth = DEPTH_MAX;
        IsWorldSpace = true;

        Texture = Ogl::ResolveTexture("test.png");

        static std::default_random_engine engine;
        engine.seed(std::time(NULL));
        static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
        distribution.reset();

        VelocityAngle = 2.0f * PI * distribution(engine);
        BallVelocity = Vec2::FromAngle(VelocityAngle) * BallSpeed;
    }

    float Clamp(float v, float min, float max)
    {
        v = v < min ? min : v;
        v = v > max ? max : v;
        return v;
    }

    Color GetGradientColor(float t)
    {
        t = fmod(t / GradientTime, 1.0f);

        if (t <= TimeStep / GradientTime)
        {
            Color oldColor = GradientColor1;
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

        Vec2 bounds = Ogl::CameraSize / 2;
        if (BallPos.X + BallSize.X > bounds.X || BallPos.X < -bounds.X)
        {
            BallPos.X = Clamp(BallPos.X, -bounds.X, bounds.X);
            VelocityAngle = PI - VelocityAngle;
            BallVelocity = Vec2::FromAngle(VelocityAngle) * BallSpeed;
        }
        if (BallPos.Y + BallSize.Y > bounds.Y || BallPos.Y < -bounds.Y)
        {
            BallPos.Y = Clamp(BallPos.Y, -bounds.Y, bounds.Y);
            VelocityAngle = -VelocityAngle;
            BallVelocity = Vec2::FromAngle(VelocityAngle) * BallSpeed;
        }
    }
};

int main()
{
    Ogl::Initialize(300, 300, "Ball", false);
    Ogl::SetCameraSize(Vec2(3.0f));

    TriangleLayer triangleLayer = {};
    Ogl::AddLayer(&triangleLayer);

    BallLayer ballLayer = {};
    Ogl::AddLayer(&ballLayer);

    Ogl::UpdateLoop();
    return 0;
}
