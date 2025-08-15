#include "ogl.hpp"

struct TextLayer : Ogl::Layer
{
    Ogl::TextureGroup Font;
    std::string Text = "Use arrows to move the camera.\nScroll to zoom in/out.\nErase with backspace, insert a new line with enter.\n:)";

    TextLayer() : Ogl::Layer()
    {
        IsWorldSpace = true;
        Redraw = true;

        Font = Ogl::ResolveFont("test.bdf");

        Ogl::Subscribe<Ogl::KeyPressEvent>(&OnKeyPress, this);
        Ogl::Subscribe<Ogl::CharacterEvent>(&OnCharacterReceived, this);
        Ogl::Subscribe<Ogl::ScrollEvent>(&OnScroll);
    }

    ~TextLayer()
    {
        Ogl::Subscribe<Ogl::KeyPressEvent>(&OnKeyPress, this);
        Ogl::Unsubscribe<Ogl::CharacterEvent>(&OnCharacterReceived, this);
        Ogl::Unsubscribe<Ogl::ScrollEvent>(&OnScroll);
    }

    static void OnKeyPress(Ogl::KeyPressEvent ev, void* data)
    {
        if (ev.Action == GLFW_RELEASE)
            return;

        TextLayer& layer = *reinterpret_cast<TextLayer*>(data);
        std::string& text = layer.Text;
        
        if (ev.Key == GLFW_KEY_ENTER)
        {
            text.push_back('\n');
            layer.Redraw = true;
        }

        if (ev.Key == GLFW_KEY_BACKSPACE && text.length() > 0)
        {
            text.resize(text.size() - 1);
            layer.Redraw = true;
        }
    }

    static void OnCharacterReceived(Ogl::CharacterEvent ev, void* data)
    {
        TextLayer& layer = *reinterpret_cast<TextLayer*>(data);
        layer.Text.push_back(ev.Utf8[0]);
        layer.Redraw = true;
    }

    static void OnScroll(Ogl::ScrollEvent ev, void* layer)
    {
        Ogl::SetCameraScale(Ogl::CameraScale + ev.OffsetY * 0.05f);
    }

    void Draw() override
    {
        if (Ogl::IsKeyPressed(GLFW_KEY_UP))
            Ogl::SetCameraPosition(Ogl::CameraPosition + Vec2(0.0f, 0.05f));

        if (Ogl::IsKeyPressed(GLFW_KEY_DOWN))
            Ogl::SetCameraPosition(Ogl::CameraPosition + Vec2(0.0f, -0.05f));

        if (Ogl::IsKeyPressed(GLFW_KEY_LEFT))
            Ogl::SetCameraPosition(Ogl::CameraPosition + Vec2(-0.05f, 0.0f));

        if (Ogl::IsKeyPressed(GLFW_KEY_RIGHT))
            Ogl::SetCameraPosition(Ogl::CameraPosition + Vec2(0.05f, 0.0f));

        if (Redraw)
            DrawText(Vec2(-7.5f, 7.0f), Text, 0.01f, Font);
    }
};

int main()
{
    Ogl::Initialize(500, 500, "Text", false);
    Ogl::SetCameraSize(Vec2(15.0f));

    TextLayer textLayer = {};
    Ogl::AddLayer(&textLayer);

    Ogl::UpdateLoop();
    return 0;
}
