#include <codecvt>
#include "tc/graphics.hpp"

namespace Tcg = Tc::Graphics;

const float PixelsPerMeter = 50.0f;

struct TextLayer : Tcg::Layer
{
    Tcg::BitmapFont Font;
    std::string Text = "Use arrows to move the camera.\nScroll to zoom in/out.\nYou can use enter, backspace and paste with ctrl + V.\n:)";
    std::wstring_convert<std::codecvt_utf8<unsigned int>, unsigned int> Utf32Converter;

    TextLayer() : Tcg::Layer(true, GL_TRIANGLES), Font(Tcg::ResolveFont("test.bdf"))
    {
        Redraw = true;

        Subscribe<Tcg::WindowResizeEvent>(&OnWindowResize);
        Subscribe<Tcg::KeyPressEvent>(&OnKeyPress);
        Subscribe<Tcg::CharacterEvent>(&OnCharacterReceived);
        Subscribe<Tcg::ScrollEvent>(&OnScroll);
    }

    static bool OnWindowResize(Tcg::WindowResizeEvent& ev, void* data)
    {
        Tcg::SetCameraSize(Tc::Vec2(ev.Width, ev.Height) / PixelsPerMeter);
        return false;
    }

    static bool OnKeyPress(Tcg::KeyPressEvent& ev, void* data)
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
            text.append(Tcg::GetClipboardContents());
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

    static bool OnCharacterReceived(Tcg::CharacterEvent& ev, void* data)
    {
        TextLayer& layer = *reinterpret_cast<TextLayer*>(data);

        //converting utf32 to utf8
        layer.Text.append(layer.Utf32Converter.to_bytes(&ev.Codepoint, &ev.Codepoint + 1));

        layer.Redraw = true;
        return true;
    }

    static bool OnScroll(Tcg::ScrollEvent& ev, void* data)
    {
        Tcg::SetCameraScale(Tcg::CameraScale + ev.OffsetY * 0.05f);
        return true;
    }

    void Draw() override
    {
        if (Tcg::IsKeyPressed(GLFW_KEY_UP))
            Tcg::SetCameraPosition(Tcg::CameraPosition + Tc::Vec2(0.0f, 0.05f));

        if (Tcg::IsKeyPressed(GLFW_KEY_DOWN))
            Tcg::SetCameraPosition(Tcg::CameraPosition + Tc::Vec2(0.0f, -0.05f));

        if (Tcg::IsKeyPressed(GLFW_KEY_LEFT))
            Tcg::SetCameraPosition(Tcg::CameraPosition + Tc::Vec2(-0.05f, 0.0f));

        if (Tcg::IsKeyPressed(GLFW_KEY_RIGHT))
            Tcg::SetCameraPosition(Tcg::CameraPosition + Tc::Vec2(0.05f, 0.0f));

        if (Redraw)
            DrawText(Tc::Vec2(0.0f), Text, 1.0f, Font);
    }
};

int main()
{
    Tcg::Initialize(500, 500, "Text");
    Tcg::SetCameraSize(Tc::Vec2(500.0f) / PixelsPerMeter);

    TextLayer textLayer = {};
    Tcg::AddLayer(&textLayer);

    Tcg::UpdateLoop();
    return 0;
}
