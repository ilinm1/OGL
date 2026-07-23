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

        TextField() {}

        //'scale' sets line height if text is multiline, otherwise it's equal to field's height
        //if 'multiline' is set line can be split by newline characters
        TextField(
            Vec2 position,
            Vec2 dimensions,
            std::string text,
            BitmapFont font,
            float scale,
            bool centerText,
            bool multiline,
            Texture texture,
            Color color,
            Color textColor) : Widget(position, dimensions), Text(text), Font(font), CenterText(centerText), Multiline(multiline), TextScale(scale),
                BaseTexture(texture), BaseColor(color), TextColor(textColor)
        {}

        void Draw() override
        {
            Vec2 centerPosition = Vec2(Position.X + (Dimensions.X - Ogl::SizeFromPixels(Font.MaxWidth, Parent->IsWorldSpace) * Text.length()).X / 2, Position.Y);
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

        Button()
        {
            Subscribe<MousePressEvent>(OnMousePress);
        }

        //'handler' is called when button is pressed and should return whether the press had actually occured to update button's appearance
        Button(
            Vec2 position,
            Vec2 dimensions,
            EventHandler<MousePressEvent> handler,
            std::string text,
            BitmapFont font,
            Color color = COLOR_WHITE,
            Color textColor = COLOR_BLACK,
            Color pressedColor = COLOR_WHITE,
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
            Vec2 mousePos = Ogl::PointFromPixels(Ogl::GetCursorPos(), widget->Parent->IsWorldSpace);

            widget->Pressed = false;
            if (ev.Action == GLFW_PRESS && IsPointInBox(mousePos, widget->Position, widget->Position + widget->Dimensions))
                widget->Pressed = widget->Handler(ev, data);

            widget->BaseColor = widget->Pressed ? widget->PressedColor : widget->DefaultColor;
            widget->TextColor = widget->Pressed ? widget->PressedTextColor : widget->DefaultTextColor;
            return widget->Pressed;
        }
    };

    struct InputField : TextField
    {
        unsigned int CursorPosition;
        float CursorBlinkPeriod; //in seconds
        std::chrono::time_point<std::chrono::steady_clock> LastBlink;

        bool InFocus;
        bool CursorVisible;

        InputField() : CursorPosition(0), LastBlink(), InFocus(false), CursorVisible(false)
        {
            Subscribe<MousePressEvent>(OnMousePress);
            Subscribe<CharacterEvent>(OnCharacterReceived);
            Subscribe<KeyPressEvent>(OnKeyPress);
        }

        //'scale' sets line height if text is multiline, otherwise it's equal to field's height
        //'blinkPeriod' specifies how often should cursor blink while field is in focus
        InputField(Vec2 position,
                   Vec2 dimensions,
                   std::string text,
                   BitmapFont font,
                   float scale,
                   bool multiline,
                   Texture texture,
                   Color color,
                   Color textColor,
                   float blinkPeriod) : TextField(position, dimensions, text, font, scale, false, multiline, texture, color, textColor),
            CursorPosition(0), CursorBlinkPeriod(blinkPeriod), LastBlink(), InFocus(false), CursorVisible(false)
        {
            Subscribe<MousePressEvent>(OnMousePress);
            Subscribe<CharacterEvent>(OnCharacterReceived);
            Subscribe<KeyPressEvent>(OnKeyPress);
        }

        //returns position of character with a given index (it's bottom left corner)
        Vec2 GetOffset(unsigned int index)
        {
            if (!GlyphPositions.size())
                return Position;

            if (index > GlyphPositions.size() - 1)
                return GlyphPositions.back() + Vec2(SizeFromPixels(Vec2(Font.MaxWidth), Parent->IsWorldSpace).X, 0);

            return GlyphPositions[index];
        }

        //returns string index for a given position (may be greater than last character index)
        unsigned int GetIndex(Vec2 offset)
        {
            float lineHeight = Multiline ? TextScale : Dimensions.Y;
            float maxGlyphWidth = SizeFromPixels(Vec2(Font.MaxWidth), Parent->IsWorldSpace).X;

            for (unsigned int i = 0; i < GlyphPositions.size(); i++)
            {
                Vec2 pos = GlyphPositions[i];
                if (offset.Y > pos.Y && offset.Y < pos.Y + lineHeight && abs(offset.X - pos.X) < maxGlyphWidth / 2)
                    return i;
            }

            return GlyphPositions.size();
        }

        static bool OnMousePress(MousePressEvent& ev, void* data)
        {
            InputField* widget = reinterpret_cast<InputField*>(data);
            Vec2 mousePos = Ogl::PointFromPixels(Ogl::GetCursorPos(), widget->Parent->IsWorldSpace);

            if (ev.Action != GLFW_PRESS)
                return false;

            if (IsPointInBox(mousePos, widget->Position, widget->Position + widget->Dimensions))
            {
                widget->InFocus = true;
                widget->CursorPosition = widget->GetIndex(mousePos);
                widget->CursorVisible = true;
                return true;
            }

            widget->InFocus = false;
            return false;
        }

        static bool OnCharacterReceived(CharacterEvent& ev, void* data)
        {
            InputField* widget = reinterpret_cast<InputField*>(data);
            static std::wstring_convert<std::codecvt_utf8<unsigned int>, unsigned int> Utf32Converter;

            if (!widget->InFocus)
                return false;

            widget->Text.insert(widget->CursorPosition++, Utf32Converter.to_bytes(&ev.Codepoint, &ev.Codepoint + 1));
            return true;
        }

        static bool OnKeyPress(KeyPressEvent& ev, void* data)
        {
            InputField* widget = reinterpret_cast<InputField*>(data);

            if (!widget->InFocus || ev.Action == GLFW_RELEASE)
                return false;

            if (ev.Key == GLFW_KEY_ENTER && widget->Multiline)
            {
                widget->Text.insert(widget->CursorPosition, "\n");
                widget->CursorPosition++;
                return true;
            }

            if (ev.Key == GLFW_KEY_V && ev.Modifiers & GLFW_MOD_CONTROL)
            {
                std::string cb = Ogl::GetClipboardContents();
                widget->Text.insert(widget->CursorPosition, cb);
                widget->CursorPosition += cb.length();
                return true;
            }

            if (ev.Key == GLFW_KEY_BACKSPACE && widget->Text.length() > 0 && widget->CursorPosition > 0)
            {
                widget->Text.erase(widget->CursorPosition - 1, 1);
                widget->CursorPosition--;
                return true;
            }

            if (ev.Key == GLFW_KEY_LEFT && widget->CursorPosition > 0)
            {
                widget->CursorPosition--;
                widget->CursorVisible = true;
                return true;
            }

            if (ev.Key == GLFW_KEY_RIGHT && widget->CursorPosition < widget->Text.length())
            {
                widget->CursorPosition++;
                widget->CursorVisible = true;
                return true;
            }

            return false;
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
                Parent->DrawRect(pos, pos + Vec2(0.01f, Multiline ? TextScale : Dimensions.Y), TextColor);
            }
        }
    };
}
