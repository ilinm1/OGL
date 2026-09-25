#pragma once

#include "tc/misc/events.hpp"
#include "context.hpp"
#include "input_events.hpp"
#include "layer.hpp"

namespace Tcg = Tc::Graphics;

namespace Tc::Graphics::Widgets
{
    struct WidgetLayer;

    struct Widget : Subscriber
    {
        Vec2 Position;
        Vec2 Dimensions;
        WidgetLayer* Parent = nullptr;

        Widget();
        Widget(Vec2 position, Vec2 dimensions);

        virtual void Draw();
    };

    struct WidgetLayer : Layer
    {
        std::vector<Widget*> Widgets;

        WidgetLayer(bool isWorldSpace = false, unsigned int primitiveType = GL_TRIANGLES, unsigned int drawingHeight = HEIGHT_MIN, size_t renderingDataSize = 256);

        void AddWidget(Widget* widgetPtr);
        void RemoveWidget(Widget* widgetPtr);
        void Draw() override;
    };

    struct TextField : Widget
    {
        std::string Text;
        BitmapFont Font;
        bool CenterText;
        bool Multiline;
        float TextScale;

        Texture BaseTexture;
        Color BaseColor;
        Color TextColor;

        std::vector<Vec2> GlyphPositions;

        //'scale' sets line height if text is multiline, otherwise it's equal to field's height
        //if 'multiline' is set line can be split by newline characters
        TextField(
            Vec2 position,
            Vec2 dimensions,
            std::string text,
            BitmapFont font,
            float scale = 0.1f,
            bool centerText = true,
            bool multiline = false,
            Texture texture = Texture{},
            Color color = COLOR_TRANSPARENT,
            Color textColor = COLOR_WHITE);

        void Draw() override;
    };

    struct Button : TextField
    {
        Color DefaultColor;
        Color DefaultTextColor;
        Color PressedColor;
        Color PressedTextColor;

        EventHandler<Tcg::MousePressEvent> Handler; //return value of the handler tells if the button is currently pressed
        bool Pressed = false;

        //'handler' is called when button is pressed and should return whether the press had actually occured to update button's appearance
        Button(
            Vec2 position,
            Vec2 dimensions,
            EventHandler<MousePressEvent> handler,
            std::string text,
            BitmapFont font,
            Color color = COLOR_WHITE,
            Color textColor = COLOR_BLACK,
            Color pressedColor = COLOR_GREY,
            Color pressedTextColor = COLOR_BLACK,
            Texture texture = Texture {},
            bool centerText = true);

        static bool OnMousePress(MousePressEvent& ev, void* data);
    };

    struct InputField : TextField
    {
        std::basic_string<unsigned int> Input;
        std::string Hint;
        static std::wstring_convert<std::codecvt_utf8<unsigned int>, unsigned int> Utf32Converter;

        Color DefaultTextColor;
        Color HintTextColor;

        unsigned int CursorPosition;
        float CursorBlinkPeriod; //in seconds
        std::chrono::time_point<std::chrono::steady_clock> LastBlink;

        bool InFocus;
        bool CursorVisible;

        InputField(Vec2 position,
                   Vec2 dimensions,
                   std::string text,
                   std::string hint,
                   BitmapFont font,
                   float scale = 0.0f,
                   bool multiline = false,
                   Texture texture = Texture{},
                   Color color = COLOR_WHITE,
                   Color textColor = COLOR_BLACK,
                   Color hintColor = COLOR_GREY,
                   float blinkPeriod = 0.5f);

        void UpdateText();
        Vec2 GetOffset(unsigned int index);
        unsigned int GetIndex(Vec2 offset);
        static bool OnMousePress(MousePressEvent& ev, void* data);
        static bool OnCharacterReceived(CharacterEvent& ev, void* data);
        static bool OnKeyPress(KeyPressEvent& ev, void* data);
        void Draw() override;
    };

    inline std::wstring_convert<std::codecvt_utf8<unsigned int>, unsigned int> InputField::Utf32Converter = {};

    struct Slider : Widget
    {
        float Value;
        float MinValue;
        float MaxValue;
        float Step;

        BitmapFont Font;
        Color SliderColor;
        Color BaseColor;
        Texture SliderTexture;
        Texture BaseTexture;

        float SliderWidth; //width of the draggable square in meters/NDC units
        float RelativeTextSize; //text's height relative to the widget height, i.e. if this is set to 0.5 then half of the widget will be occupied by the slider and the other half by text underneath
        unsigned int StepsToDraw;
        
        bool Dragging = false;

        Slider(
            Vec2 position,
            Vec2 dimensions,
            float value,
            float minValue,
            float maxValue,
            float step,
            float sliderWidth,
            BitmapFont font,
            Color sliderColor = COLOR_WHITE,
            Color baseColor = COLOR_GREY,
            Texture sliderTexture = Texture{},
            Texture baseTexture = Texture{},
            float relativeTextSize = 0.5f,
            unsigned int stepsToDraw = 3);

        Vec2 GetSliderBasePosition();
        Vec2 ValueToPosition(float value);
        float PositionToValue(Vec2 pos);
        static bool OnMousePress(MousePressEvent& ev, void* data);
        static bool OnScroll(ScrollEvent& ev, void* data);
        void Draw() override;
    };
}
