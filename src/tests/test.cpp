#include <format>
#include <random>
#include "../ogl.hpp"

#define PI 3.1415

struct TriangleLayer : Ogl::Layer
{
    Ogl::TextureData Texture;
    Ogl::TextureGroup Font;

    TriangleLayer() : Ogl::Layer()
    {
        Font = Ogl::ResolveFont("test.bdf");
        Texture = Ogl::ResolveTexture("test.png");
    }

    void Draw() override
    {
        bool input = false;

        if (Ogl::IsKeyPressed(GLFW_KEY_A))
        {
            input = true;
            Ogl::SetCameraPosition(Ogl::CameraPosition + Vec2(0.05f, 0.0f));
        }

        if (Ogl::IsKeyPressed(GLFW_KEY_D))
        {
            input = true;
            Ogl::SetCameraPosition(Ogl::CameraPosition + Vec2(-0.05f, 0.0f));
        }

        if (Ogl::IsKeyPressed(GLFW_KEY_W))
        {
            input = true;
            Ogl::SetCameraScale(Ogl::CameraScale + 0.01f);
        }

        if (Ogl::IsKeyPressed(GLFW_KEY_S))
        {
            input = true;
            Ogl::SetCameraScale(Ogl::CameraScale - 0.01f);
        }

        if (input)
            DrawText(Vec2(-1.0f), std::format("Scale: {}%", static_cast<int>(Ogl::CameraScale * 100)), 0.5f, Font);

        DrawTriangle(Vec2(-0.75f, -0.75f), Vec2(0.75f, -0.75f), Vec2(0.0f, 0.75f), Texture);
    }
};

struct BallLayer : Ogl::Layer
{
    bool ResourcesLoaded = false;
    Ogl::TextureData Texture;

    Vec2 BallPos = Vec2(0.0f);
    Vec2 BallVelocity;
    float VelocityAngle;

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

    void Draw() override
    {
        DrawRect(BallPos, BallPos + BallSize, Texture);
        BallPos += BallVelocity * TimeStep;

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
    Ogl::Initialize(300, 300, "Test", false);
    Ogl::SetCameraSize(Vec2(3.0f));

    TriangleLayer triangleLayer = {};
    Ogl::AddLayer(&triangleLayer);

    BallLayer ballLayer = {};
    Ogl::AddLayer(&ballLayer);

    Ogl::UpdateLoop();
    return 0;
}
