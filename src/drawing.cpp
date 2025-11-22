#include <codecvt>
#include <ogl.hpp>

//drawing methods

//writes 'count' vertices to the buffer 'buf' of size 'size'
//null can be passed to 'texCoords' and 'colors' parameters to omit them
void Ogl::Layer::WriteVertexData(const Vec2* coords, const Vec2* texCoords, const Color* colors, Texture texture, size_t count)
{
    if (RenderingDataSize - RenderingDataUsed < count * VERT_SIZE)
    {
        char* oldData = RenderingData;
        size_t oldSize = RenderingDataSize;
        RenderingDataSize = RenderingDataSize * 2 + count * VERT_SIZE;
        RenderingData = new char[RenderingDataSize];
        std::memcpy(RenderingData, oldData, oldSize);
        delete[] oldData;
    }

    char* data = RenderingData + RenderingDataUsed;
    for (int i = 0; i < count; i++)
    {
        //vertex coordinates - xy
        *reinterpret_cast<float*>(data + VERT_SIZE * i) = coords[i].X;
        *reinterpret_cast<float*>(data + VERT_SIZE * i + 1 * sizeof(float)) = coords[i].Y;

        //texture coordinates - xy
        *reinterpret_cast<float*>(data + VERT_SIZE * i + 2 * sizeof(float)) = texCoords == NULL ? 0 : texCoords[i].X;
        *reinterpret_cast<float*>(data + VERT_SIZE * i + 3 * sizeof(float)) = texCoords == NULL ? 0 : texCoords[i].Y;

        //texture index
        *reinterpret_cast<unsigned int*>(data + VERT_SIZE * i + 4 * sizeof(float)) = texture.Index;

        //modulate color
        *reinterpret_cast<unsigned int*>(data + VERT_SIZE * i + 4 * sizeof(float) + sizeof(unsigned int)) = colors == NULL ? 0 : colors[i].Uint;
    }

    RenderingDataUsed += count * VERT_SIZE;
}

//draws a triangle from three points in world/screen space (depending on layer's space) with the specified texture
//'color' is modulate color (alpha can be set to zero to ignore it)
//if 'matchResolution' is set the texture will be matched to it's real resolution, otherwise stretched to fully fit the triangle
void Ogl::Layer::DrawTriangle(Vec2 a, Vec2 b, Vec2 c, Color color, Texture texture, bool matchResolution)
{
    const Vec2 coords[3] = { a, b, c };

    Vec2 max = Vec2::Max(Vec2::Max(a, b), c);
    Vec2 min = Vec2::Min(Vec2::Min(a, b), c);
    Vec2 aabb = max - min;

    TextureDimensions dimensions = Ogl::TextureDimensionsVector[texture.Index];
    Vec2 texSize = Vec2(1);
    if (matchResolution)
    {
        texSize = PointToPixels(aabb, IsWorldSpace);
        texSize.X /= dimensions.Width;
        texSize.Y /= dimensions.Height;
    }

    const Vec2 texCoords[3] =
    {  
        { (a.X - min.X) * texSize.X / aabb.X, (a.Y - min.Y) * texSize.Y / aabb.Y },
        { (b.X - min.X) * texSize.X / aabb.X, (b.Y - min.Y) * texSize.Y / aabb.Y },
        { (c.X - min.X) * texSize.X / aabb.X, (c.Y - min.Y) * texSize.Y / aabb.Y }
    };

    const Color colors[3] = { color, color, color };

    WriteVertexData(coords, texCoords, colors, texture, 3);
    AabbMax = Vec2::Max(AabbMax, max);
    AabbMin = Vec2::Min(AabbMin, min);
}

//draws a rectangle from two points in world/screen space (depending on layer's space) with the specified texture
//'color' is modulate color (alpha can be set to zero to ignore it)
//if 'matchResolution' is set the texture will be matched to it's real resolution, otherwise stretched to fully fit the rectangle
void Ogl::Layer::DrawRect(Vec2 a, Vec2 b, Color color, Texture texture, bool matchResolution)
{
    const Vec2 coords[6] =
    {
        a,
        {a.X, b.Y},
        b,
        b,
        {b.X, a.Y},
        a
    };

    TextureDimensions dimensions = Ogl::TextureDimensionsVector[texture.Index];
    Vec2 texSize = Vec2(1);
    if (matchResolution)
    {
        texSize = PointToPixels((a - b).Abs(), IsWorldSpace);
        texSize.X /= dimensions.Width;
        texSize.Y /= dimensions.Height;
    }

    const Vec2 texCoords[6] =
    {
        {0,         0},         //first triangle
        {0,         texSize.Y},
        {texSize.X, texSize.Y},
        {texSize.X, texSize.Y}, //second triangle
        {texSize.X, 0},
        {0,         0}
    };

    const Color colors[6] = { color, color, color, color, color, color };

    WriteVertexData(coords, texCoords, colors, texture, 6);
    AabbMax = Vec2::Max(AabbMax, Vec2::Max(a, b));
    AabbMin = Vec2::Min(AabbMin, Vec2::Min(a, b));
}

//expects a utf8 string
//'scale' sets the amount of NDC/in-world meters per glyph pixel
//'color' is modulate color (alpha can be set to zero to ignore it)
//if 'multiline' is set then new line will be created after reading newline
void Ogl::Layer::DrawText(Vec2 pos, std::string text, float scale, BitmapFont& font, Color color, bool multiline)
{
    static std::wstring_convert<std::codecvt_utf8<unsigned int>, unsigned int> utf8converter;
    std::basic_string<unsigned int> textUtf32 = utf8converter.from_bytes(text);

    Vec2 currentPos = pos;
    for (unsigned int codepoint : textUtf32)
    {
        if (codepoint == '\n')
        {
            currentPos.X = pos.X;
            currentPos.Y -= font.MaxHeight * scale;
            continue;
        }

        size_t index = -1;
        for (auto& [startCodepoint, endCodepoint, startIndex] : font.EncodingRanges)
        {
            if (codepoint >= startCodepoint && codepoint <= endCodepoint)
            {
                index = startIndex + codepoint - startCodepoint;
                break;
            }
        }

        if (index == -1)
            throw std::runtime_error("Character unsupported by font.");

        Texture characterTexture = Textures[index];
        TextureDimensions dimensions = Ogl::TextureDimensionsVector[characterTexture.Index];
        Vec2 characterSize = Vec2(dimensions.Width, dimensions.Height) * scale;

        DrawRect(currentPos, currentPos + Vec2(characterSize.X, characterSize.Y), color, characterTexture);
        currentPos.X += characterSize.X;
    }

    Vec2 topLeft = Vec2(pos.X, pos.Y - font.MaxHeight * scale);
    AabbMax = Vec2::Max(AabbMax, Vec2::Max(topLeft, currentPos));
    AabbMin = Vec2::Min(AabbMin, Vec2::Min(topLeft, currentPos));
}
