#include "ogl/ogl.hpp"
#include "ogl/widgets.hpp"

struct CalculatorLayer : Ogl::Widgets::WidgetLayer
{
	Ogl::Widgets::InputField* InputA;
	Ogl::Widgets::InputField* InputB;
	Ogl::Widgets::TextField* Output;

	CalculatorLayer()
	{
		Ogl::BitmapFont& font = Ogl::ResolveFont("test.bdf");

		InputA = new Ogl::Widgets::InputField(
			Ogl::Vec2(-1.0f, 0.7f),
			Ogl::Vec2(0.6f, 0.2f),
			"2",
			"A",
			font);
		AddWidget(InputA);

		AddWidget(new Ogl::Widgets::TextField(
			Ogl::Vec2(-0.4f, 0.7f),
			Ogl::Vec2(0.2f),
			"+",
			font,
			1.0f,
			true,
			false,
			Ogl::Texture{},
			COLOR_TRANSPARENT,
			COLOR_WHITE));

		InputB = new Ogl::Widgets::InputField(
			Ogl::Vec2(-0.2f, 0.7f),
			Ogl::Vec2(0.6f, 0.2f),
			"2",
			"B",
			font);
		AddWidget(InputB);

		Output = new Ogl::Widgets::TextField(
			Ogl::Vec2(0.4f, 0.7f),
			Ogl::Vec2(0.6f, 0.2f),
			"=4",
			font,
			1.0f,
			false,
			false,
			Ogl::Texture{},
			COLOR_TRANSPARENT,
			COLOR_WHITE);
		AddWidget(Output);

		AddWidget(new Ogl::Widgets::Button(
			Ogl::Vec2(-0.9f),
			Ogl::Vec2(1.8f, 0.2f),
			&OnCalculateButtonPress,
			"Calculate",
			font));
	}

	~CalculatorLayer()
	{
		for (Ogl::Widgets::Widget* widget : Widgets)
		{
			delete widget;
		}
	}

	static bool OnCalculateButtonPress(Ogl::MousePressEvent& ev, void* data)
	{
		CalculatorLayer* layer = reinterpret_cast<CalculatorLayer*>(reinterpret_cast<Ogl::Widgets::Button*>(data)->Parent);

		float a, b;

		try
		{
			a = std::stof(layer->InputA->Text);
		}
		catch (std::invalid_argument)
		{
			layer->InputA->BaseColor = Ogl::Color(255, 200, 200);
			return true;
		}

		try
		{
			b = std::stof(layer->InputB->Text);
		}
		catch (std::invalid_argument)
		{
			layer->InputB->BaseColor = Ogl::Color(255, 200, 200);
			return true;
		}

		layer->Output->Text = std::format("={}", a + b);
		layer->InputA->BaseColor = layer->InputB->BaseColor = COLOR_WHITE;
		return true;
	}
};

int main()
{
	Ogl::Initialize(300, 300, "Calculator", false);
	CalculatorLayer layer;
	Ogl::AddLayer(&layer);
	Ogl::UpdateLoop();
	return 0;
}
