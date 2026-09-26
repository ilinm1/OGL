#include "tc/graphics.hpp"

namespace Tcg = Tc::Graphics;

struct CalculatorLayer : Tcg::Widgets::WidgetLayer
{
	Tcg::Widgets::InputField* InputA;
	Tcg::Widgets::InputField* InputB;
	Tcg::Widgets::TextField* Output;

	CalculatorLayer()
	{
		Tcg::BitmapFont& font = Tcg::ResolveFont("test.bdf");

		InputA = new Tcg::Widgets::InputField(
			Tc::Vec2(-1.0f, 0.7f),
			Tc::Vec2(0.6f, 0.2f),
			"2",
			"A",
			font);
		AddWidget(InputA);

		AddWidget(new Tcg::Widgets::TextField(
			Tc::Vec2(-0.4f, 0.7f),
			Tc::Vec2(0.2f),
			"+",
			font,
			1.0f,
			true,
			false,
			Tcg::Texture{},
			COLOR_TRANSPARENT,
			COLOR_WHITE));

		InputB = new Tcg::Widgets::InputField(
			Tc::Vec2(-0.2f, 0.7f),
			Tc::Vec2(0.6f, 0.2f),
			"2",
			"B",
			font);
		AddWidget(InputB);

		Output = new Tcg::Widgets::TextField(
			Tc::Vec2(0.4f, 0.7f),
			Tc::Vec2(0.6f, 0.2f),
			"=4",
			font,
			1.0f,
			false,
			false,
			Tcg::Texture{},
			COLOR_TRANSPARENT,
			COLOR_WHITE);
		AddWidget(Output);

		AddWidget(new Tcg::Widgets::Button(
			Tc::Vec2(-0.9f),
			Tc::Vec2(1.8f, 0.2f),
			&OnCalculateButtonPress,
			"Calculate",
			font));

		AddWidget(new Tcg::Widgets::Slider(
			Tc::Vec2(-0.9f, -0.6f),
			Tc::Vec2(1.8f, 0.2f),
			0.0f,
			0.0f,
			100.0f,
			10.0f,
			0.2f,
			font
		));
	}

	~CalculatorLayer()
	{
		for (Tcg::Widgets::Widget* widget : Widgets)
		{
			delete widget;
		}
	}

	static bool OnCalculateButtonPress(Tcg::MousePressEvent& ev, void* data)
	{
		CalculatorLayer* layer = reinterpret_cast<CalculatorLayer*>(reinterpret_cast<Tcg::Widgets::Button*>(data)->Parent);

		float a, b;

		try
		{
			a = std::stof(layer->InputA->Text);
		}
		catch (std::invalid_argument)
		{
			layer->InputA->BaseColor = Tc::Color(255, 200, 200);
			return true;
		}

		try
		{
			b = std::stof(layer->InputB->Text);
		}
		catch (std::invalid_argument)
		{
			layer->InputB->BaseColor = Tc::Color(255, 200, 200);
			return true;
		}

		layer->Output->Text = std::format("={}", a + b);
		layer->InputA->BaseColor = layer->InputB->BaseColor = COLOR_WHITE;
		return true;
	}
};

int main()
{
	Tcg::Initialize(300, 300, "Calculator", false, false);
	CalculatorLayer layer;
	Tcg::AddLayer(&layer);
	Tcg::UpdateLoop();
	return 0;
}
