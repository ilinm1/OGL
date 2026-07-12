#pragma once

#include <vector>
#include <vec2.hpp>

namespace Ogl
{
	struct Rect
	{
		unsigned int X = 0; //set only by the packer
		unsigned int Y = 0;

		unsigned int Width = 0;
		unsigned int Height = 0;

		std::tuple<long, long> Data; //not used for packing
		//would've used 'dynamic_cast' and pointers to rect here instead of this ugly tuple but then classes derived from 'Rect' should be polymorphic which they have no reason to be
	};

	struct RectanglePacker
	{
		std::vector<Rect> Rects; //new rects to be packed should be put here
		std::vector<Rect> PackedRects; //all the rects that have been packed are here
		std::vector<std::tuple<unsigned int, unsigned int, unsigned int>> VerticalLevels = {}; //pieces of free space where rects can be placed; first two values are it's x/y coordinates, third value is it's width
		unsigned int TotalWidth = 0;
		unsigned int TotalHeight = 0;

		//not very efficient but it's good enough
		void Pack()
		{
			std::sort(Rects.begin(), Rects.end(), [](Rect rect1, Rect rect2) { return rect1.Width > rect2.Width; });

			for (Rect& rect : Rects)
			{
				int selectedIndex; //index of the selected vertical level
				int deltaX, deltaY; //difference in packing space dimensions
				int minDelta = std::numeric_limits<int>().max();
				VerticalLevels.push_back({ TotalWidth, 0, rect.Width }); //there's always an option to place rect at the rightmost point

				for (int i = 0; i < VerticalLevels.size(); i++) //going through the vertical levels trying to minimize difference in packing space area
				{
					auto [x, y, width] = VerticalLevels[i];

					if (width < rect.Width) //level is too narrow
						continue;

					int dx = x + rect.Width - TotalWidth; dx = std::max(dx, 0);
					int dy = y + rect.Height - TotalHeight; dy = std::max(dy, 0);
					int ds = dx * TotalHeight + dy * TotalWidth + dx * dy;

					if (ds < minDelta) //new best option
					{
						selectedIndex = i;
						deltaX = dx;
						deltaY = dy;
						minDelta = ds;
					}
				}

				TotalWidth += deltaX;
				TotalHeight += deltaY;

				auto& [selectedX, selectedY, selectedWidth] = VerticalLevels[selectedIndex];
				rect.X = selectedX;
				rect.Y = selectedY;

				if (selectedIndex != VerticalLevels.size() - 1) //if the option to put new rect at the rightmost point wasn't chosen it should be removed as to not clog the list
					VerticalLevels.pop_back();

				selectedX += rect.Width; //shrinking the selected level
				selectedWidth -= rect.Width;
				if (selectedWidth == 0) //if the entire level was used it's removed from the list
					VerticalLevels.erase(VerticalLevels.begin() + selectedIndex);
				VerticalLevels.push_back({ rect.X, rect.Y + rect.Height, rect.Width }); //adding new level above the newly packed rect

				PackedRects.push_back(rect);
			}
		}
	};
}
