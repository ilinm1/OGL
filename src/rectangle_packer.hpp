#pragma once

#include <vector>
#include "vec2.hpp"

struct Rect
{
	unsigned int X = 0; //set only by the packer
	unsigned int Y = 0;

	unsigned int Width = 0;
	unsigned int Height = 0;

	std::tuple<int, int, int, int> Data; //not used for packing
};

struct RectanglePacker
{
	std::vector<Rect> Rects;
	std::vector<Rect> PackedRects;
	std::vector<std::tuple<unsigned int, unsigned int, unsigned int>> HeightLevels = {};
	unsigned int TotalWidth = 0;
	unsigned int TotalHeight = 0;

	//not very efficient but it's good enough
	void Pack()
	{
		std::sort(Rects.begin(), Rects.end(), [](Rect rect1, Rect rect2) { return rect1.Width > rect2.Width; });

		for (Rect& rect : Rects)
		{
			int selectedIndex;
			int deltaX, deltaY;
			int minDelta = std::numeric_limits<int>().max();
			HeightLevels.push_back({ TotalWidth, 0, rect.Width }); //there's always an option to place rect at the rightmost point

			for (int i = 0; i < HeightLevels.size(); i++)
			{
				auto [x, y, width] = HeightLevels[i];

				if (width < rect.Width)
					continue;

				int dx = x + rect.Width - TotalWidth; dx = dx < 0 ? 0 : dx;
				int dy = y + rect.Height - TotalHeight; dy = dy < 0 ? 0 : dy;
				int ds = dx * TotalHeight + dy * TotalWidth + dx * dy;

				if (ds < minDelta)
				{
					selectedIndex = i;
					deltaX = dx;
					deltaY = dy;
					minDelta = ds;
				}
			}

			TotalWidth += deltaX;
			TotalHeight += deltaY;

			auto& [selectedX, selectedY, selectedWidth] = HeightLevels[selectedIndex];
			rect.X = selectedX;
			rect.Y = selectedY;

			if (selectedIndex != HeightLevels.size() - 1)
				HeightLevels.pop_back();

			selectedX += rect.Width;
			selectedWidth -= rect.Width;
			if (selectedWidth == 0)
				HeightLevels.erase(HeightLevels.begin() + selectedIndex);
			HeightLevels.push_back({ selectedX, selectedY, rect.Width });

			PackedRects.push_back(rect);
		}
	}
};
