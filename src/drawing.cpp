#include "ogl.hpp"

//drawing methods

//writes 'count' vertices to the buffer 'buf' of size 'size'
void Ogl::Layer::WriteVertexData(const Vec2* coords, const Vec2* texCoords, TextureData texture, size_t count)
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
        *reinterpret_cast<float*>(data + VERT_SIZE * i + 2 * sizeof(float)) = texCoords[i].X;
        *reinterpret_cast<float*>(data + VERT_SIZE * i + 3 * sizeof(float)) = texCoords[i].Y;

        //texture index
        *reinterpret_cast<unsigned int*>(data + VERT_SIZE * i + 4 * sizeof(float)) = texture.Index;
    }

    RenderingDataUsed += count * VERT_SIZE;
}

//draws a triangle from three points in world/screen space (depending on layer's space) with the specified texture
//if 'matchResolution' is set the texture will be matched to it's real resolution, otherwise stretched to fully fit the triangle
void Ogl::Layer::DrawTriangle(Vec2 a, Vec2 b, Vec2 c, TextureData texture, bool matchResolution)
{
    const Vec2 coords[3] = { a, b, c };

    Vec2 max, min, aabb;
    max.X = std::max(std::max(a.X, b.X), c.X);
    max.Y = std::max(std::max(a.Y, b.Y), c.Y);
    min.X = std::min(std::min(a.X, b.X), c.X);
    min.Y = std::min(std::min(a.Y, b.Y), c.Y);
    aabb = max - min;

    Vec2 texSize = Vec2(1);
    if (matchResolution)
    {
        texSize = SizeToPixels(aabb, IsWorldSpace);
        texSize.X /= texture.Width;
        texSize.Y /= texture.Height;
    }

    const Vec2 texCoords[3] =
    {  
        { (a.X - min.X) * texSize.X / aabb.X, (a.Y - min.Y) * texSize.Y / aabb.Y },
        { (b.X - min.X) * texSize.X / aabb.X, (b.Y - min.Y) * texSize.Y / aabb.Y },
        { (c.X - min.X) * texSize.X / aabb.X, (c.Y - min.Y) * texSize.Y / aabb.Y }
    };

    WriteVertexData(coords, texCoords, texture, 3);
}

//draws a rectangle from two points in world/screen space (depending on layer's space) with the specified texture
//if 'matchResolution' is set the texture will be matched to it's real resolution, otherwise stretched to fully fit the rectangle
void Ogl::Layer::DrawRect(Vec2 a, Vec2 b, TextureData texture, bool matchResolution)
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

    Vec2 texSize = Vec2(1);
    if (matchResolution)
    {
        texSize = SizeToPixels((a - b).Abs(), IsWorldSpace);
        texSize.X /= texture.Width;
        texSize.Y /= texture.Height;
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

    WriteVertexData(coords, texCoords, texture, 6);
}

//ascii only
//if 'multiline' is set then new line will be created after reading newline
void Ogl::Layer::DrawText(Vec2 pos, std::string text, float scale, TextureGroup font, bool multiline)
{
    if (IsWorldSpace)
        scale /= CameraScale;

    int x = 0;
    for (char character : text)
    {
        if (character == '\n')
        {
            pos -= SizeFromPixels(Vec2(0, font.MaxHeight), IsWorldSpace) * scale;
            x = 0;
            continue;
        }

        unsigned int characterIndex = character - 0x20;
        characterIndex = characterIndex < 0 ? 0 : characterIndex;
        characterIndex = characterIndex > font.Size - 1 ? 0 : characterIndex;

        TextureData characterTexture = Textures[font.Index + characterIndex];
        Vec2 characterSize = SizeFromPixels(Vec2(characterTexture.Width, characterTexture.Height), IsWorldSpace) * scale;

        DrawRect(pos + Vec2(characterSize.X * x, 0), pos + Vec2(characterSize.X * (x + 1), characterSize.Y), characterTexture);
        x++;
    }
}
