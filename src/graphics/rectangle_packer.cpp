#include <algorithm>
#include "tc/graphics/rectangle_packer.hpp"

namespace Tcg = Tc::Graphics;

//this implementation tries to minimise total area of the texture at every step, without trying to balance it's width and height
//as such this is probably not the most efficient way to pack an atlas but it works
void Tcg::RectanglePacker::Pack()
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

			if (x + rect.Width > MaxWidth || y + rect.Height > MaxHeight) //out of bounds
				continue;

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
