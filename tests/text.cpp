#include <codecvt>
#include <ogl.hpp>

const float PixelsPerMeter = 50.0f; //can't define in class

struct TextLayer : Ogl::Layer
{
    Ogl::BitmapFont Font;
    std::string Text = "Use arrows to move the camera.\nScroll to zoom in/out.\nYou can use enter, backspace and paste with ctrl + V.\n:)";
    std::wstring_convert<std::codecvt_utf8<unsigned int>, unsigned int> Utf32Converter;

    TextLayer() : Ogl::Layer()
    {
        IsWorldSpace = true;
        Redraw = true;

        Font = Ogl::ResolveFont("test.bdf");

        Ogl::Subscribe<Ogl::WindowResizeEvent>(&OnWindowResize, this);
        Ogl::Subscribe<Ogl::KeyPressEvent>(&OnKeyPress, this);
        Ogl::Subscribe<Ogl::CharacterEvent>(&OnCharacterReceived, this);
        Ogl::Subscribe<Ogl::ScrollEvent>(&OnScroll, this);
    }

    static void OnWindowResize(Ogl::WindowResizeEvent ev, void* data, bool& handled)
    {
        Ogl::SetCameraSize(Vec2(ev.Width, ev.Height) / PixelsPerMeter);
    }

    static void OnKeyPress(Ogl::KeyPressEvent ev, void* data, bool& handled)
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

        if (ev.Key == GLFW_KEY_V && ev.Modifiers & GLFW_MOD_CONTROL)
        {
            text.append(Ogl::GetClipboardContents());
            layer.Redraw = true;
        }

        if (ev.Key == GLFW_KEY_BACKSPACE && text.length() > 0)
        {
            text.resize(text.size() - 1);
            layer.Redraw = true;
        }
    }

    static void OnCharacterReceived(Ogl::CharacterEvent ev, void* data, bool& handled)
    {
        TextLayer& layer = *reinterpret_cast<TextLayer*>(data);

        //converting utf32 to utf8
        layer.Text.append(layer.Utf32Converter.to_bytes(&ev.Codepoint, &ev.Codepoint + 1));

        layer.Redraw = true;
    }

    static void OnScroll(Ogl::ScrollEvent ev, void* data, bool& handled)
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
            DrawText(Vec2(0.0f), Text, 0.01f, Font);
    }
};

int main()
{
    Ogl::Initialize(500, 500, "Text", false);
    Ogl::SetCameraSize(Vec2(500.0f) / PixelsPerMeter);

    TextLayer textLayer = {};
    Ogl::AddLayer(&textLayer);

    Ogl::UpdateLoop();
    return 0;
}
