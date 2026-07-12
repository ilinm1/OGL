#pragma once

#include <vector>
#include <string>
#include "ogl.hpp"

namespace Ogl::Widgets
{
    bool IsPointInBox(Vec2 point, Vec2 lb, Vec2 rt)
    {
        return point.X > lb.X && point.X < rt.X && point.Y > lb.Y && point.Y < rt.Y;
    }

    struct Button : Widget
    {
        std::string Text;
        BitmapFont Font;
        Texture BaseTexture;
        bool CenterText;

        Color BaseColor;
        Color TextColor;
        Color PressedColor;
        Color PressedTextColor;

        EventHandler<MousePressEvent> Handler; //return value of the handler tells if the button is currently pressed
        bool Pressed = false;

        Button() 
        {
            Subscribe<MousePressEvent>(OnMousePress);
        }

        Button(
            Vec2 position,
            Vec2 dimensions,
            std::string text,
            BitmapFont font,
            Color color = COLOR_WHITE,
            Color textColor = COLOR_BLACK,
            Color pressedColor = COLOR_WHITE,
            Color pressedTextColor = COLOR_BLACK,
            Texture texture = Texture{},
            bool centerText = true,
            EventHandler<MousePressEvent> handler = nullptr) : Widget(position, dimensions)
        {
            Text = text;
            Font = font;
            BaseColor = color;
            TextColor = textColor;
            PressedColor = pressedColor;
            PressedTextColor = pressedTextColor;
            BaseTexture = texture;
            CenterText = centerText;
            Handler = handler;

            Subscribe<MousePressEvent>(OnMousePress);
        }

        static bool OnMousePress(MousePressEvent& ev, void* data)
        {
            Button* widget = reinterpret_cast<Button*>(data);

            Vec2 mousePos = Ogl::PointFromPixels(Ogl::GetCursorPos(), widget->Parent->IsWorldSpace);
            if (IsPointInBox(mousePos, widget->Position, widget->Position + widget->Dimensions))
            {
                widget->Pressed = widget->Handler(ev, data);
                return true;
            }

            return false;
        }

        void Draw() override
        {
            Vec2 centerPosition = Vec2(Position.X + (Dimensions.X - Ogl::SizeFromPixels(Font.MaxWidth, Parent->IsWorldSpace) * Text.length()).X / 2, Position.Y);
            Parent->DrawRect(Position, Position + Dimensions, Pressed ? PressedColor : BaseColor, BaseTexture);
            Parent->DrawText(
                CenterText ? centerPosition : Position,
                Text,
                Dimensions.Y,
                Font,
                Pressed ? PressedTextColor : TextColor,
                false,
                true,
                true,
                Dimensions.X,
                Dimensions.Y);
        }
    };
}
