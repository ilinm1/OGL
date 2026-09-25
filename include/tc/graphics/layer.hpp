#pragma once

#include <codecvt>
#include "GLFW/glfw3.h"
#include "tc/misc/color.hpp"
#include "tc/misc/events.hpp"
#include "tc/misc/vec2.hpp"
#include "texture.hpp"

#define VERT_SIZE (4 * sizeof(float) + 2 * sizeof(unsigned int))
#define HEIGHT_MAX 0xFFFFFFFF
#define HEIGHT_MIN 0

//'Layer' class

namespace Tc::Graphics
{
    //rendering layer, each layer owns a block of video memory
    struct Layer : Subscriber
    {
        size_t BlockIndex; //index of the block of video memory owned by this layer
        size_t Id; //mostly for logging purpouses, never repeat

        unsigned int PrimitiveType; //most drawing methods use GL_TRIANGLES, each layer can only use one primitive per draw call
        unsigned int DrawingHeight; //DO NOT SET DIRECTLY, USE 'SetLayerHeight'. layers with higher height will be drawn before layers with lower height (on top of em)
        bool IsWorldSpace; //if set objects drawn by the layer will be transformed to NDC from world coordinates by the vertex shader
        bool Redraw = false; //if set data from the previous 'Draw' call will be discarded even if nothing was generated during the last call; will be reset afterwards
        bool IsOutOfView = false; //if set layer is currently out of view and won't be drawn

        Vec2 AabbMax = Vec2(0); //AABB of objects drawn by the layer, used for clipping (if enabled), WILL NOT BE SET WHEN USING 'WriteVertexData' DIRECTLY
        Vec2 AabbMin = Vec2(0);

        size_t RenderingDataSize = 0;
        size_t RenderingDataUsed = 0;
        char* RenderingData = nullptr;

        Layer(bool isWorldSpace = false, unsigned int primitiveType = GL_TRIANGLES, unsigned int drawingHeight = HEIGHT_MIN, size_t renderingDataSize = 256);

        //each draw call generates new primitives to be drawn, replacing the old ones; if no new ones were generated the old ones will be drawn
        virtual void Draw();

        virtual ~Layer();

        void WriteVertexData(const Vec2* coords, const Vec2* texCoords, const Color* colors, Texture texture, size_t count);
        void DrawTriangle(Vec2 a, Vec2 b, Vec2 c, Color color = COLOR_WHITE, Texture texture = Texture{}, bool matchResolution = false);
        void DrawRect(Vec2 a, Vec2 b, Color color = COLOR_WHITE, Texture texture = Texture{}, bool matchResolution = false, bool mirrorX = false, bool mirrorY = false, bool swapXY = false);
        std::vector<Vec2> DrawText(Vec2 pos, std::string text, float scale, BitmapFont& font, Color color = COLOR_WHITE, bool matchResolution = false, bool multiline = true, bool bounded = false, float maxWidth = 1.0f, float maxHeight = 1.0f);
        std::vector<Vec2> DrawText(Vec2 pos, std::basic_string<unsigned int> text, float scale, BitmapFont& font, Color color, bool matchResolution, bool multiline, bool bounded, float maxWidth, float maxHeight);
        void DrawLine(Vec2 a, Vec2 b, Color color = COLOR_WHITE);
    };
}
