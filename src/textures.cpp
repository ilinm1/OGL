#include <cstring>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <glad/glad.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <ogl.hpp>
#include <rectangle_packer.hpp>

//texture methods

Ogl::Texture AddTexture(std::filesystem::path path, Rect rect)
{
    Ogl::TextureDimensionsVector.push_back({ rect.X, rect.Y, rect.Width, rect.Height });
    Ogl::TexturesToUpdate.push_back(Ogl::Textures.size());
    Ogl::Textures.push_back({ path, Ogl::Textures.size() });
    return Ogl::Textures.back();
}

void UpdateTextureData()
{
    size_t maxIndex = *std::max_element(Ogl::TexturesToUpdate.begin(), Ogl::TexturesToUpdate.end());
    size_t requiredSsboSize = (maxIndex + 1) * sizeof(Ogl::TextureDimensions);
    if (Ogl::Ssbo.Size < requiredSsboSize)
        throw std::runtime_error("Out of video memory.");

    for (size_t index : Ogl::TexturesToUpdate)
    {
        Ogl::TextureDimensions dimensions = Ogl::TextureDimensionsVector[index];
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, index * sizeof(Ogl::TextureDimensions), sizeof(Ogl::TextureDimensions), &dimensions);
    }

    Ogl::TexturesToUpdate.clear();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Ogl::AtlasWidth, Ogl::AtlasHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, Ogl::AtlasData);
}

void InitializeAtlas()
{
    glGenTextures(1, &Ogl::Atlas);
    glBindTexture(GL_TEXTURE_2D, Ogl::Atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

//data pointing to the top-left pixel of the image, x & y specifying the left-bottom corner of the image area
//if 'flip' is set the image will be flipped vertically (since opengl treats first pixel as bottom-left loaded images will be displayed upside-down)
void WriteToAtlas(unsigned char* data, unsigned int x, unsigned int y, unsigned int width, unsigned int height, bool flip = true)
{
    if (x + width > Ogl::AtlasWidth || y + height > Ogl::AtlasHeight)
        throw std::runtime_error("Tried to write out of atlas bounds.");

    for (int i = 0; i < height; i++)
    {
        unsigned int dataRow = flip ? height - i - 1 : i;
        std::memcpy(
            Ogl::AtlasData + (Ogl::AtlasWidth * (i + y) + x) * IMAGE_CHANNELS,
            data + width * dataRow * IMAGE_CHANNELS,
            width * IMAGE_CHANNELS);
    }
}

//writes zeroes to each channel of every pixel in the specified area
void ZeroAtlas(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
    if (x + width > Ogl::AtlasWidth || y + height > Ogl::AtlasHeight)
        throw std::runtime_error("Tried to write out of atlas bounds.");

    y = Ogl::AtlasHeight - y - height;

    for (int i = 0; i < height; i++)
    {
        std::memset(Ogl::AtlasData + (Ogl::AtlasWidth * (i + y) + x) * IMAGE_CHANNELS, 0, width * IMAGE_CHANNELS);
    }
}

void ResizeAtlas(unsigned int width, unsigned int height)
{
    if (Ogl::AtlasWidth == width && Ogl::AtlasHeight == height)
        return;

    unsigned char* data = new unsigned char[width * height * IMAGE_CHANNELS];

    unsigned char* oldData = Ogl::AtlasData;
    unsigned int oldWidth = Ogl::AtlasWidth;
    unsigned int oldHeight = Ogl::AtlasHeight;

    Ogl::AtlasWidth = width;
    Ogl::AtlasHeight = height;
    Ogl::AtlasData = data;

    if (oldData != NULL)
    {
        WriteToAtlas(oldData, 0, 0, std::min(oldWidth, width), std::min(oldHeight, height), false);
        delete[] oldData;
    }
}

//loads textures from the specified paths, adding them to atlas
std::vector<Ogl::Texture> Ogl::LoadTextures(std::vector<std::filesystem::path> paths)
{
    if (Atlas == 0)
        InitializeAtlas();

    std::vector<Ogl::Texture> result;

    //getting texture rects
    for (int i = 0; i < paths.size(); i++)
    {
        std::filesystem::path path = paths[i];

        if (!std::filesystem::exists(path))
            throw std::runtime_error(std::format("Invalid texture path: '{}'.", path.string()));

        int width, height, components;
        stbi_info(path.string().c_str(), &width, &height, &components);
        AtlasPacker.Rects.push_back({ .Width = static_cast<unsigned int>(width), .Height = static_cast<unsigned int>(height), .Data = { i, 0, 0, 0 } });
    }

    //packing newly generated rects & resizing the atlas
    AtlasPacker.Pack();
    ResizeAtlas(AtlasPacker.TotalWidth, AtlasPacker.TotalHeight);

    //loading new textures & writing them onto the atlas
    for (Rect rect : AtlasPacker.Rects)
    {
        int rectId = get<0>(rect.Data);
        std::filesystem::path path = paths[rectId];

        int _;
        unsigned char* data = stbi_load(path.string().c_str(), &_, &_, &_, IMAGE_CHANNELS);
        if (data == NULL)
            throw std::runtime_error(std::format("STBI error: '{}'.", stbi_failure_reason()));

        WriteToAtlas(data, rect.X, rect.Y, rect.Width, rect.Height);
        delete[] data;

        result.push_back(AddTexture(path, rect));
    }
    AtlasPacker.Rects.clear();
    
    //updating texture data array & sending everything to GPU
    UpdateTextureData();

    return result;
}

//barebones BDF font loader
Ogl::BitmapFont Ogl::LoadBdfFont(std::filesystem::path path)
{
    BitmapFont result = {};

    if (Atlas == 0)
        InitializeAtlas();

    if (!std::filesystem::exists(path))
        throw std::runtime_error(std::format("Invalid font path: '{}'.", path.string()));

    std::fstream file = std::fstream(path, std::ios::in | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error(std::format("Failed to open font file: '{}'.", path.string()));

    //reading glyph data
    //'Rect' data is used for storing encoding, bitmap starting byte, x offset, y offset (in that order)
    std::string line;
    const int maxOffset = 256; //max x/y offset
    int code, width, height, offsetX, offsetY;
    while (!file.eof())
    {
        std::getline(file, line);

        //font end
        if (line.rfind("ENDFONT") == 0)
            break;

        //new glyph
        if (line.rfind("STARTCHAR") == 0)
            AtlasPacker.Rects.emplace_back();

        //settings glyph's encoding
        if (line.rfind("ENCODING") == 0)
        {
            sscanf(line.substr(8).data(), "%d", &code);
            get<0>(AtlasPacker.Rects.back().Data) = code;
        }

        //glyph's bitmap size & offsets
        if (line.rfind("BBX") == 0)
        {
            Rect& glyphRect = AtlasPacker.Rects.back();
            sscanf(line.substr(3).data(), "%d %d %d %d", &width, &height, &offsetX, &offsetY);

            if (offsetX > maxOffset || offsetY > maxOffset)
                throw std::runtime_error("Glyph X/Y offset is too high.");

            glyphRect.Width = width + std::abs(offsetX), 
            glyphRect.Height = height + std::abs(offsetY);

            if (glyphRect.Width > result.MaxWidth)
                result.MaxWidth = glyphRect.Width;

            if (glyphRect.Height > result.MaxHeight)
                result.MaxHeight = glyphRect.Height;

            get<2>(glyphRect.Data) = std::max(-offsetX, 0);
            get<3>(glyphRect.Data) = std::max(-offsetY, 0);
        }

        //glyph's bitmap
        if (line.rfind("BITMAP") == 0)
            get<1>(AtlasPacker.Rects.back().Data) = file.tellg();
    }

    //packing glyph rects to atlas & resizing it
    AtlasPacker.Pack();
    ResizeAtlas(AtlasPacker.TotalWidth, AtlasPacker.TotalHeight);

    //writing bitmaps to atlas & setting encoding ranges
    size_t rangeStartIndex = 0;
    bool firstGlyph = true;
    unsigned int rangeStartCodepoint = 0;
    unsigned int prevCodepoint = 0;

    //sorting by encoding
    std::sort(AtlasPacker.Rects.begin(), AtlasPacker.Rects.end(), [](Rect rect1, Rect rect2) { return get<0>(rect1.Data) < get<0>(rect2.Data); });

    for (Rect rect : AtlasPacker.Rects)
    {
        unsigned int currentCodepoint = get<0>(rect.Data);

        if (firstGlyph || currentCodepoint != prevCodepoint + 1)
        {
            if (firstGlyph)
            {
                firstGlyph = false;
            }
            else 
            {
                result.EncodingRanges.push_back({ rangeStartCodepoint, prevCodepoint, rangeStartIndex });
            }

            rangeStartCodepoint = currentCodepoint;
            rangeStartIndex = Textures.size();
        }

        prevCodepoint = currentCodepoint;

        ZeroAtlas(rect.X, rect.Y, rect.Width, rect.Height); //to avoid random garbage on edges of glyphs with non-zero offsets
        file.seekg(get<1>(rect.Data)); //going to the start of the bitmap

        unsigned char buffer[IMAGE_CHANNELS * 4]; //4 cause we're writing up to 4 pixels per hexadecimal digit
        int offsetX = get<2>(rect.Data);
        int offsetY = get<3>(rect.Data);

        for (int y = rect.Height - offsetY - 1; y > 0; y--)
        {
            std::getline(file, line);

            for (int x = 0; x < line.length(); x++)
            {
                char character = line[x];
                if (character < '0') //ignoring whitespaces
                    continue;

                unsigned char value = character < '9' ? character - '0' : character - 'A' + 10; //hexadecimal digit to 4 bit value

                int pixels = rect.Width - offsetX - x * 4;
                pixels = std::max(std::min(pixels, 4), 0);

                for (int i = 0; i < pixels; i++)
                {
                    *reinterpret_cast<unsigned int*>(buffer + i * IMAGE_CHANNELS) = value & (0b1000 >> i) ? std::numeric_limits<unsigned int>().max() : 0;
                }

                if (pixels != 0)
                    WriteToAtlas(buffer, rect.X + offsetX + x * 4, y, pixels, 1);
            }
        }

        AddTexture(path, rect);
    }

    result.Path = path;
    result.GlyphCount = AtlasPacker.Rects.size();
    result.EncodingRanges.push_back({ rangeStartCodepoint, prevCodepoint, rangeStartIndex });

    file.close();
    AtlasPacker.Rects.clear();
    UpdateTextureData();

    return result;
}

//loads all the textures from the specified path (recursively)
std::vector<Ogl::Texture> Ogl::LoadTexturesFromPath(std::filesystem::path path)
{
    std::vector<std::filesystem::path> paths;

    for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(path))
    {
        std::string ext = entry.path().extension().string();
        for (std::string imageExt : IMAGE_EXTS)
        {
            if (ext == imageExt)
                paths.push_back(entry.path());
        }
    }

    return LoadTextures(paths);
}

//finds texture by path if it's already loaded/loads it if not
Ogl::Texture Ogl::ResolveTexture(std::filesystem::path path)
{
    if (std::filesystem::exists(path))
    {
        for (int i = 1; i < Textures.size(); i++)
        {
            Texture& texture = Textures[i];
            if (std::filesystem::equivalent(texture.Path, path))
                return texture;
        }
    }

    return LoadTextures({ path })[0];
}

//finds font by path if it's already loaded/loads it if not
Ogl::BitmapFont Ogl::ResolveFont(std::filesystem::path path)
{
    if (std::filesystem::exists(path))
    {
        for (BitmapFont& font : Fonts)
        {
            if (std::filesystem::equivalent(font.Path, path))
                return font;
        }
    }

    return LoadBdfFont(path);
}
