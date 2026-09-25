#pragma once

#include <vector>
#include "tc/misc/vec2.hpp"

namespace Tc::Graphics
{
	struct Rect
	{
		unsigned int X = 0; //set only by the packer
		unsigned int Y = 0;

		unsigned int Width = 0;
		unsigned int Height = 0;

		std::tuple<long, long> Data; //not used for packing
		//would've used 'dynamic_cast' and pointers to rect here instead of this ugly tuple but 1.) rects are used differently in different parts of code
		//and 2.) classes derived from 'Rect' must be polymorphic which they have no reason to be
	};

	struct RectanglePacker
	{
		std::vector<Rect> Rects; //new rects to be packed should be put here
		std::vector<Rect> PackedRects; //all the rects that have been packed are here
		std::vector<std::tuple<unsigned int, unsigned int, unsigned int>> VerticalLevels = {}; //pieces of free space where rects can be placed; first two values are it's x/y coordinates, third value is it's width
		unsigned int MaxWidth = 0;
		unsigned int MaxHeight = 0;
		unsigned int TotalWidth = 0;
		unsigned int TotalHeight = 0;

		void Pack();
	};
}
