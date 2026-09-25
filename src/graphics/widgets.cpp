#include <vector>
#include <string>
#include <codecvt>
#include <chrono>
#include "tc/misc/vec2.hpp"
#include "tc/graphics/widgets.hpp"

namespace Tcw = Tc::Graphics::Widgets;

//utility methods

bool IsPointInBox(Tc::Vec2 point, Tc::Vec2 lb, Tc::Vec2 rt)
{
    return point.X > lb.X && point.X < rt.X && point.Y > lb.Y && point.Y < rt.Y;
}

float Clamp(float v, float min, float max)
{
    v = v < min ? min : v;
    v = v > max ? max : v;
    return v;
}

//widget

Tcw::Widget::Widget() : Position(0), Dimensions(0) {}

Tcw::Widget::Widget(Vec2 position, Vec2 dimensions) : Position(position), Dimensions(dimensions) {}

void Tcw::Widget::Draw() {}

//widget layer

Tcw::WidgetLayer::WidgetLayer(
    bool isWorldSpace,
    unsigned int primitiveType,
    unsigned int drawingHeight,
    size_t renderingDataSize) :
    Layer(isWorldSpace, primitiveType, drawingHeight, renderingDataSize) {}

void Tcw::WidgetLayer::AddWidget(Widget* widgetPtr)
{
    widgetPtr->Parent = this;
    Widgets.push_back(widgetPtr);
}

void Tcw::WidgetLayer::RemoveWidget(Widget* widgetPtr)
{
    widgetPtr->Parent = nullptr;
    Widgets.erase(std::find(Widgets.begin(), Widgets.end(), widgetPtr));
}

void Tcw::WidgetLayer::Draw()
{
    for (Widget* widget : Widgets)
    {
        widget->Draw();
    }
}

//text field

//'scale' sets line height if text is multiline, otherwise it's equal to field's height
//if 'multiline' is set line can be split by newline characters
Tcw::TextField::TextField(
    Vec2 position,
    Vec2 dimensions,
    std::string text,
    BitmapFont font,
    float scale,
    bool centerText,
    bool multiline,
    Texture texture,
    Color color,
    Color textColor) :
    Widget(position, dimensions),
    Text(text),
    Font(font),
    CenterText(centerText),
    Multiline(multiline),
    TextScale(scale),
    BaseTexture(texture),
    BaseColor(color),
    TextColor(textColor) {}

void Tcw::TextField::Draw()
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

//button

//'handler' is called when button is pressed and should return whether the press had actually occured to update button's appearance
Tcw::Button::Button(
    Vec2 position,
    Vec2 dimensions,
    EventHandler<MousePressEvent> handler,
    std::string text,
    BitmapFont font,
    Color color,
    Color textColor,
    Color pressedColor,
    Color pressedTextColor,
    Texture texture,
    bool centerText) : TextField(position, dimensions, text, font, 1.0f, centerText, false, texture, color, textColor),
    DefaultColor(color), DefaultTextColor(textColor), PressedColor(pressedColor), PressedTextColor(pressedTextColor), Handler(handler)
{
    Subscribe<MousePressEvent>(OnMousePress);
}

bool Tcw::Button::OnMousePress(MousePressEvent& ev, void* data)
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

//input field

//'hint' is displayed when nothing is entered
//'scale' sets line height if text is multiline, otherwise it's equal to field's height
//'blinkPeriod' specifies how often should cursor blink while field is in focus (in seconds)
Tcw::InputField::InputField(Vec2 position,
    Vec2 dimensions,
    std::string text,
    std::string hint,
    BitmapFont font,
    float scale,
    bool multiline,
    Texture texture,
    Color color,
    Color textColor,
    Color hintColor,
    float blinkPeriod) :
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

void Tcw::InputField::UpdateText()
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
Tc::Vec2 Tcw::InputField::GetOffset(unsigned int index)
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
unsigned int Tcw::InputField::GetIndex(Vec2 offset)
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

bool Tcw::InputField::OnMousePress(MousePressEvent& ev, void* data)
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

bool Tcw::InputField::OnCharacterReceived(CharacterEvent& ev, void* data)
{
    InputField* widget = reinterpret_cast<InputField*>(data);

    if (!widget->Parent || !widget->InFocus)
        return false;

    widget->Input.insert(widget->CursorPosition++, 1, ev.Codepoint);
    widget->UpdateText();
    return true;
}

bool Tcw::InputField::OnKeyPress(KeyPressEvent& ev, void* data)
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

void Tcw::InputField::Draw()
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

//slider

Tcw::Slider::Slider(
    Vec2 position,
    Vec2 dimensions,
    float value,
    float minValue,
    float maxValue,
    float step,
    float sliderWidth,
    BitmapFont font,
    Color sliderColor,
    Color baseColor,
    Texture sliderTexture,
    Texture baseTexture,
    float relativeTextSize,
    unsigned int stepsToDraw) :
    Widget(position, dimensions), Value(value), MinValue(minValue), MaxValue(maxValue), Step(step), SliderWidth(sliderWidth),
    Font(font), SliderColor(sliderColor), BaseColor(baseColor), SliderTexture(sliderTexture), BaseTexture(baseTexture), RelativeTextSize(relativeTextSize), StepsToDraw(stepsToDraw)
{
    if (MaxValue <= MinValue)
        throw std::runtime_error("Slider's maximal value should be greater than it's minimal value.");

    Subscribe<MousePressEvent>(OnMousePress);
    Subscribe<ScrollEvent>(OnScroll);
}

Tc::Vec2 Tcw::Slider::GetSliderBasePosition()
{
    return Position + Vec2(0, Dimensions.Y * RelativeTextSize);
}

Tc::Vec2 Tcw::Slider::ValueToPosition(float value)
{
    value = Clamp(value, MinValue, MaxValue);
    value = std::floor(value / Step) * Step;
    return GetSliderBasePosition() + Vec2(Dimensions.X - SliderWidth, 0) * ((value - MinValue) / (MaxValue - MinValue));
}

float Tcw::Slider::PositionToValue(Vec2 pos)
{
    float value = ((pos - Position).X / (Dimensions.X - SliderWidth)) * (MaxValue - MinValue) + MinValue;
    value = std::floor(value / Step) * Step;
    return Clamp(value, MinValue, MaxValue);
}

bool Tcw::Slider::OnMousePress(MousePressEvent& ev, void* data)
{
    Slider* widget = reinterpret_cast<Slider*>(data);
    if (!widget->Parent)
        return false;

    Vec2 mousePos = PointFromPixels(GetCursorPos(), widget->Parent->IsWorldSpace);
    widget->Dragging = IsPointInBox(mousePos, widget->Position, widget->Position + widget->Dimensions) && ev.Action == GLFW_PRESS;
    return false;
}

bool Tcw::Slider::OnScroll(ScrollEvent& ev, void* data)
{
    Slider* widget = reinterpret_cast<Slider*>(data);
    if (!widget->Parent)
        return false;

    Vec2 mousePos = PointFromPixels(GetCursorPos(), widget->Parent->IsWorldSpace);
    if (IsPointInBox(mousePos, widget->Position, widget->Position + widget->Dimensions))
        widget->Value = Clamp(widget->Value + widget->Step * (ev.OffsetY > 0 ? 1.0f : -1.0f), widget->MinValue, widget->MaxValue);

    return false;
}

void Tcw::Slider::Draw()
{
    Parent->DrawRect(GetSliderBasePosition(), GetSliderBasePosition() + Vec2(Dimensions.X, Dimensions.Y * (1.0f - RelativeTextSize)), BaseColor, BaseTexture);
    Vec2 sliderPos = ValueToPosition(Value);
    Parent->DrawRect(sliderPos, sliderPos + Vec2(SliderWidth, Dimensions.Y * (1.0f - RelativeTextSize)), SliderColor, SliderTexture);

    if (Dragging)
    {
        Vec2 mousePos = PointFromPixels(GetCursorPos(), Parent->IsWorldSpace);
        Value = PositionToValue(mousePos);
        Parent->DrawText(ValueToPosition(Value) + Vec2(0.0f, Dimensions.Y * (1.0f - RelativeTextSize)), std::format("{:.1f}", Value), RelativeTextSize * Dimensions.Y, Font, SliderColor.WithAlpha(128)); //drawing text above the slider
    }

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
