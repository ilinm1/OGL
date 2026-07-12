#include <codecvt>
#include "ogl/ogl.hpp"

const float PixelsPerMeter = 50.0f;

struct TextLayer : Ogl::Layer
{
    Ogl::BitmapFont Font;
    std::string Text = "Use arrows to move the camera.\nScroll to zoom in/out.\nYou can use enter, backspace and paste with ctrl + V.\n:)";
    std::wstring_convert<std::codecvt_utf8<unsigned int>, unsigned int> Utf32Converter;

    TextLayer()
    {
        IsWorldSpace = true;
        Redraw = true;

        Font = Ogl::ResolveFont("test.bdf");

        Subscribe<Ogl::WindowResizeEvent>(&OnWindowResize);
        Subscribe<Ogl::KeyPressEvent>(&OnKeyPress);
        Subscribe<Ogl::CharacterEvent>(&OnCharacterReceived);
        Subscribe<Ogl::ScrollEvent>(&OnScroll);
    }

    static bool OnWindowResize(Ogl::WindowResizeEvent& ev, void* data)
    {
        Ogl::SetCameraSize(Ogl::Vec2(ev.Width, ev.Height) / PixelsPerMeter);
        return false;
    }

    static bool OnKeyPress(Ogl::KeyPressEvent& ev, void* data)
    {
        if (ev.Action == GLFW_RELEASE)
            return false;

        TextLayer& layer = *reinterpret_cast<TextLayer*>(data);
        std::string& text = layer.Text;
        
        if (ev.Key == GLFW_KEY_ENTER)
        {
            text.push_back('\n');
            layer.Redraw = true;
            return true;
        }

        if (ev.Key == GLFW_KEY_V && ev.Modifiers & GLFW_MOD_CONTROL)
        {
            text.append(Ogl::GetClipboardContents());
            layer.Redraw = true;
            return true;
        }

        if (ev.Key == GLFW_KEY_BACKSPACE && text.length() > 0)
        {
            text.resize(text.size() - 1);
            layer.Redraw = true;
            return true;
        }

        return false;
    }

    static bool OnCharacterReceived(Ogl::CharacterEvent& ev, void* data)
    {
        TextLayer& layer = *reinterpret_cast<TextLayer*>(data);

        //converting utf32 to utf8
        layer.Text.append(layer.Utf32Converter.to_bytes(&ev.Codepoint, &ev.Codepoint + 1));

        layer.Redraw = true;
        return true;
    }

    static bool OnScroll(Ogl::ScrollEvent& ev, void* data)
    {
        Ogl::SetCameraScale(Ogl::CameraScale + ev.OffsetY * 0.05f);
        return true;
    }

    void Draw() override
    {
        if (Ogl::IsKeyPressed(GLFW_KEY_UP))
            Ogl::SetCameraPosition(Ogl::CameraPosition + Ogl::Vec2(0.0f, 0.05f));

        if (Ogl::IsKeyPressed(GLFW_KEY_DOWN))
            Ogl::SetCameraPosition(Ogl::CameraPosition + Ogl::Vec2(0.0f, -0.05f));

        if (Ogl::IsKeyPressed(GLFW_KEY_LEFT))
            Ogl::SetCameraPosition(Ogl::CameraPosition + Ogl::Vec2(-0.05f, 0.0f));

        if (Ogl::IsKeyPressed(GLFW_KEY_RIGHT))
            Ogl::SetCameraPosition(Ogl::CameraPosition + Ogl::Vec2(0.05f, 0.0f));

        if (Redraw)
            DrawText(Ogl::Vec2(0.0f), Text, 1.0f, Font);
    }
};

int main()
{
    Ogl::Initialize(500, 500, "Text", false);
    Ogl::SetCameraSize(Ogl::Vec2(500.0f) / PixelsPerMeter);

    TextLayer textLayer = {};
    Ogl::AddLayer(&textLayer);

    Ogl::UpdateLoop();
    return 0;
}
