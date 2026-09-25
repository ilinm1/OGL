#pragma once

namespace Tc::Graphics
{
    struct WindowResizeEvent
    {
        int Width;
        int Height;
    };

    struct KeyPressEvent
    {
        int Key;
        int Scancode;
        int Action;
        int Modifiers;
    };

    struct CharacterEvent
    {
        unsigned int Codepoint; //utf32 codepoint
    };

    struct MousePressEvent
    {
        int Button;
        int Action;
        int Modifiers;
    };

    struct ScrollEvent
    {
        double OffsetX;
        double OffsetY;
    };
}
