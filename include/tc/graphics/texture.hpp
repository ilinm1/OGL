#pragma once

#include <vector>
#include <filesystem>

//'Texture', 'TextureDimensions' and 'BitmapFont' classes

namespace Tc::Graphics
{
    //relative to atlas
    struct TextureDimensions
    {
        unsigned int X = 0;
        unsigned int Y = 0;
        unsigned int Width = 0;
        unsigned int Height = 0;
    };

    struct Texture
    {
        std::filesystem::path Path;
        size_t Index = 0; //index in 'Textures' and 'TextureDimensionsVector'; if index is zero then texture is invalid

        bool IsValid()
        {
            return Index != 0;
        }
    };

    struct BitmapFont
    {
        std::filesystem::path Path;
        unsigned int MaxWidth; //in pixels
        unsigned int MaxHeight;

        size_t GlyphCount = 0;
        std::vector<std::tuple<unsigned int, unsigned int, size_t>> EncodingRanges; //first utf32 codepoint, second codepoint, first glyph index (to avoid having a glyph for every utf codepoint)

        bool IsValid()
        {
            return GlyphCount != 0;
        }
    };
}
