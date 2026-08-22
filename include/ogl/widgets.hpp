#pragma once

#include <vector>
#include <string>
#include <codecvt>
#include <chrono>
#include "ogl.hpp"

namespace Ogl::Widgets
{
    bool IsPointInBox(Vec2 point, Vec2 lb, Vec2 rt)
    {
        return point.X > lb.X && point.X < rt.X && point.Y > lb.Y && point.Y < rt.Y;
    }

    float Clamp(float v, float min, float max)
    {
        v = v < min ? min : v;
        v = v > max ? max : v;
        return v;
    }

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
            float scale,
            bool centerText = true,
            bool multiline = false,
            Texture texture = Texture{},
            Color color = COLOR_WHITE,
            Color textColor = COLOR_BLACK) :
            Widget(position, dimensions),
            Text(text),
            Font(font),
            CenterText(centerText),
            Multiline(multiline),
            TextScale(scale),
            BaseTexture(texture),
            BaseColor(color),
            TextColor(textColor)
        {}

        void Draw() override
        {
            float maxGlyphWidth = static_cast<float>(Font.MaxWidth) / Font.MaxHeight * (Multiline ? TextScale : Dimensions.Y);
            Vec2 centerPosition = Vec2(Position.X + (Dimensions.X - maxGlyphWidth * Text.length()) / 2, Position.Y);
            Parent->DrawRect(Position, Position + Dimensions, BaseColor, BaseTexture);
            GlyphPositions = Parent->DrawText(
                CenterText ? centerPosition : Position,
                Text,
                Multiline ? TextScale : Dimensions.Y,
                Font,
                TextColor,
                false,
                Multiline,
                true,
                Dimensions.X,
                Dimensions.Y);
        }
    };

    struct Button : TextField
    {
        Color DefaultColor;
        Color DefaultTextColor;
        Color PressedColor;
        Color PressedTextColor;

        EventHandler<MousePressEvent> Handler; //return value of the handler tells if the button is currently pressed
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
            Texture texture = Texture{},
            bool centerText = true) : TextField(position, dimensions, text, font, 1.0f, centerText, false, texture, color, textColor),
                DefaultColor(color), DefaultTextColor(textColor), PressedColor(pressedColor), PressedTextColor(pressedTextColor), Handler(handler)
        {
            Subscribe<MousePressEvent>(OnMousePress);
        }

        static bool OnMousePress(MousePressEvent& ev, void* data)
        {
            Button* widget = reinterpret_cast<Button*>(data);
            if (!widget->Parent)
                return false;

            Vec2 mousePos = PointFromPixels(GetCursorPos(), widget->Parent->IsWorldSpace);
            widget->Pressed = false;
            if (ev.Action == GLFW_PRESS && IsPointInBox(mousePos, widget->Position, widget->Position + widget->Dimensions))
                widget->Pressed = widget->Handler(ev, data);

            widget->BaseColor = widget->Pressed ? widget->PressedColor : widget->DefaultColor;
            widget->TextColor = widget->Pressed ? widget->PressedTextColor : widget->DefaultTextColor;
            return false;
        }
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

        //'hint' is displayed when nothing is entered
        //'scale' sets line height if text is multiline, otherwise it's equal to field's height
        //'blinkPeriod' specifies how often should cursor blink while field is in focus (in seconds)
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
                   float blinkPeriod = 0.5f) : 
            TextField(position, dimensions, text.empty() ? hint : text, font, scale, false, multiline, texture, color, text.empty() ? hintColor : textColor),
            CursorPosition(0),
            CursorBlinkPeriod(blinkPeriod),
            LastBlink(),
            Input(Utf32Converter.from_bytes(text)),
            Hint(hint),
            DefaultTextColor(textColor),
            HintTextColor(hintColor),
            InFocus(false),
            CursorVisible(false)
        {
            Subscribe<MousePressEvent>(OnMousePress);
            Subscribe<CharacterEvent>(OnCharacterReceived);
            Subscribe<KeyPressEvent>(OnKeyPress);
        }

        void UpdateText()
        {
            if (Input.empty())
            {
                Text = Hint;
                TextColor = HintTextColor;
            }
            else
            {
                Text = Utf32Converter.to_bytes(Input);
                TextColor = DefaultTextColor;
            }
        }

        //returns position of character with a given index (it's bottom left corner)
        Vec2 GetOffset(unsigned int index)
        {
            if (Input.empty())
                return Position;

            if (index > GlyphPositions.size() - 1)
            {
                float maxGlyphWidth = static_cast<float>(Font.MaxWidth) / Font.MaxHeight * (Multiline ? TextScale : Dimensions.Y);
                return GlyphPositions.back() + Vec2(maxGlyphWidth, 0);
            }

            return GlyphPositions[index];
        }

        //returns input string index for a given position (may be greater than last character index)
        unsigned int GetIndex(Vec2 offset)
        {
            if (Input.empty())
                return 0;

            float lineHeight = Multiline ? TextScale : Dimensions.Y;
            float maxGlyphWidth = static_cast<float>(Font.MaxWidth) / Font.MaxHeight * lineHeight;

            for (unsigned int i = 0; i < GlyphPositions.size(); i++)
            {
                Vec2 pos = GlyphPositions[i];
                if (offset.Y > pos.Y && offset.Y < pos.Y + lineHeight && abs(offset.X - pos.X) < maxGlyphWidth / 2)
                    return i;
            }

            return Input.size();
        }

        static bool OnMousePress(MousePressEvent& ev, void* data)
        {
            InputField* widget = reinterpret_cast<InputField*>(data);

            if (widget->Parent == nullptr)
                return false;

            if (ev.Action != GLFW_PRESS)
                return false;

            Vec2 mousePos = PointFromPixels(GetCursorPos(), widget->Parent->IsWorldSpace);
            if (IsPointInBox(mousePos, widget->Position, widget->Position + widget->Dimensions))
            {
                widget->InFocus = true;
                widget->CursorPosition = widget->GetIndex(mousePos);
                widget->CursorVisible = true;
                return false;
            }

            widget->InFocus = false;
            return false;
        }

        static bool OnCharacterReceived(CharacterEvent& ev, void* data)
        {
            InputField* widget = reinterpret_cast<InputField*>(data);

            if (!widget->Parent || !widget->InFocus)
                return false;

            widget->Input.insert(widget->CursorPosition++, 1, ev.Codepoint);
            widget->UpdateText();
            return true;
        }

        static bool OnKeyPress(KeyPressEvent& ev, void* data)
        {
            InputField* widget = reinterpret_cast<InputField*>(data);

            if (!widget->Parent || !widget->InFocus || ev.Action == GLFW_RELEASE)
                return false;

            bool handled = false;

            if (ev.Key == GLFW_KEY_ENTER && widget->Multiline)
            {
                widget->Input.insert(widget->CursorPosition++, 1, '\n');
                handled = true;
            }

            if (ev.Key == GLFW_KEY_V && ev.Modifiers & GLFW_MOD_CONTROL)
            {
                std::basic_string<unsigned int> cb = widget->Utf32Converter.from_bytes(GetClipboardContents());
                widget->Input.insert(widget->CursorPosition, cb);
                widget->CursorPosition += cb.size();
                handled = true;
            }

            if (ev.Key == GLFW_KEY_BACKSPACE && widget->Input.length() > 0 && widget->CursorPosition > 0)
            {
                widget->Input.erase(widget->CursorPosition - 1, 1);
                widget->CursorPosition--;
                handled = true;
            }

            if (ev.Key == GLFW_KEY_LEFT && widget->CursorPosition > 0)
            {
                widget->CursorPosition--;
                widget->CursorVisible = true;
                handled = true;
            }

            if (ev.Key == GLFW_KEY_RIGHT && widget->CursorPosition < widget->Input.length())
            {
                widget->CursorPosition++;
                widget->CursorVisible = true;
                handled = true;
            }

            widget->UpdateText();
            return handled;
        }

        void Draw() override
        {
            std::chrono::time_point<std::chrono::steady_clock> time = std::chrono::steady_clock::now();
            if (static_cast<std::chrono::duration<float>>(time - LastBlink).count() >= CursorBlinkPeriod)
            {
                LastBlink = time;
                CursorVisible = !CursorVisible;
            }

            TextField::Draw();

            if (InFocus && CursorVisible)
            {
                Vec2 pos = GetOffset(CursorPosition);
                Parent->DrawRect(pos, pos + Vec2(0.005f, Multiline ? TextScale : Dimensions.Y), TextColor);
            }
        }
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

        float SliderWidth;
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
            float relativeTextSize = 0.3f,
            unsigned int stepsToDraw = 3) :
            Widget(position, dimensions), Value(value), MinValue(minValue), MaxValue(maxValue), Step(step), SliderWidth(sliderWidth), 
            Font(font), SliderColor(sliderColor), BaseColor(baseColor), SliderTexture(sliderTexture), BaseTexture(baseTexture), RelativeTextSize(relativeTextSize), StepsToDraw(stepsToDraw)
        {
            if (MaxValue <= MinValue)
                throw std::runtime_error("Slider's maximal value should be greater than it's minimal value.");

            Subscribe<MousePressEvent>(OnMousePress);
            Subscribe<ScrollEvent>(OnScroll);
        }

        Vec2 GetSliderBasePosition()
        {
            return Position + Vec2(0, Dimensions.Y * RelativeTextSize);
        }

        Vec2 ValueToPosition(float value)
        {
            value = Clamp(value, MinValue, MaxValue);
            value = std::floor(value / Step) * Step;
            return GetSliderBasePosition() + Vec2(Dimensions.X - SliderWidth, 0) * ((value - MinValue) / (MaxValue - MinValue));
        }

        float PositionToValue(Vec2 pos)
        {
            float value = ((pos - Position).X / (Dimensions.X - SliderWidth)) * (MaxValue - MinValue) + MinValue;
            value = std::floor(value / Step) * Step;
            return Clamp(value, MinValue, MaxValue);
        }

        static bool OnMousePress(MousePressEvent& ev, void* data)
        {
            Slider* widget = reinterpret_cast<Slider*>(data);
            if (!widget->Parent)
                return false;

            Vec2 mousePos = PointFromPixels(GetCursorPos(), widget->Parent->IsWorldSpace);
            widget->Dragging = IsPointInBox(mousePos, widget->Position, widget->Position + widget->Dimensions) && ev.Action == GLFW_PRESS;
            return false;
        }

        static bool OnScroll(ScrollEvent& ev, void* data)
        {
            Slider* widget = reinterpret_cast<Slider*>(data);
            if (!widget->Parent)
                return false;

            Vec2 mousePos = PointFromPixels(GetCursorPos(), widget->Parent->IsWorldSpace);
            if (IsPointInBox(mousePos, widget->Position, widget->Position + widget->Dimensions))
                widget->Value = Clamp(widget->Value + widget->Step * (ev.OffsetY > 0 ? 1.0f : -1.0f), widget->MinValue, widget->MaxValue);
            
            return false;
        }

        void Draw() override
        {
            Parent->DrawRect(GetSliderBasePosition(), GetSliderBasePosition() + Vec2(Dimensions.X, Dimensions.Y * (1.0f - RelativeTextSize)), BaseColor, BaseTexture);
            Vec2 sliderPos = ValueToPosition(Value);
            Parent->DrawRect(sliderPos, sliderPos + Vec2(SliderWidth, Dimensions.Y * (1.0f - RelativeTextSize)), SliderColor, SliderTexture);

            if (Dragging)
                Value = PositionToValue(PointFromPixels(GetCursorPos(), Parent->IsWorldSpace));

            for (unsigned int i = 0; i < StepsToDraw; i++)
            {
                float value = i * (MaxValue - MinValue) / (StepsToDraw - 1) + MinValue;
                Vec2 pos = ValueToPosition(value);
                Parent->DrawText(
                    pos - Vec2(0, RelativeTextSize * Dimensions.Y),
                    std::format("{:.1f}", value),
                    Dimensions.Y * RelativeTextSize,
                    Font,
                    SliderColor,
                    false,
                    false);
            }
        }
    };
}
