#pragma once

#include <cstddef>
#include <string>
#include <filesystem>
#include <GLFW/glfw3.h>
#include "vec2.hpp"
#include "mat3.hpp"
#include "rectangle_packer.hpp"

#define IMAGE_CHANNELS 4 //rgba, just to avoid magic numbers
#define VERT_SIZE (4 * sizeof(float) + sizeof(unsigned int))
#define BUFFER_SIZE (VERT_SIZE * 3 * 100000)

#define TEXTURE_MAX 256
#define IMAGE_EXTS { ".png", ".jpeg", ".bmp" }

//real depth is a float, equal to layer's depth divided by this value
//that's the maximal value for 16 bit depth buffer, though 24 and 32 bit buffers may support even more different depths
#define DEPTH_MAX 65535
#define DEPTH_MIN 0

namespace Ogl
{
    struct TextureData
    {
        std::filesystem::path Path;
        unsigned int Width = 0;
        unsigned int Height = 0;
        unsigned int AtlasX = 0; //position inside of the texture atlas, in pixels
        unsigned int AtlasY = 0;
        size_t Index = 0; //index in 'Textures' and 'TextureDataArray'
    };

    //for animations/fonts
    struct TextureGroup
    {
        std::filesystem::path Path;
        size_t Size = 0;
        size_t Index = 0;
    };

    //renderer layer, each layer owns a block of video memory which it can edit in order to draw stuff (i.e. background, menu, terrain, objects, etc.)
    //drawing order is random but drawing depth allows to specify which layers should be on top of others (higher depth layers are drawn ontop of lower ones)
    struct Layer
    {
        //block of buffer memory owned by this layer
        size_t BlockOffset = 0; //offset in video memory, set by the renderer
        size_t BlockSize = 0; //total block size, initially set by the layer and updated by the renderer during reallocations
        size_t BlockUsed = 0; //size that's actually being used, set by the renderer

        unsigned int DrawingDepth = DEPTH_MIN;
        bool IsWorldSpace = false;
        bool SavePreviousData = true; //if set data from the previous 'Draw' call will be displayed, otherwise all of it will be discarded

        size_t RenderingDataSize = 0;
        size_t RenderingDataUsed = 0;
        char* RenderingData = NULL;

        Layer(size_t renderingDataSize = 256)
        {
            RenderingData = new char[renderingDataSize];
            RenderingDataSize = renderingDataSize;
        }

        virtual void Draw() {}

        friend bool operator==(const Layer& l, const Layer& r)
        {
            return l.BlockOffset == r.BlockOffset;
        }

        virtual ~Layer() 
        {
            delete[] RenderingData;
        }

        //drawing methods

        void WriteVertexData(const Vec2* coords, const Vec2* texCoords, TextureData texture, size_t count);
        void DrawTriangle(Vec2, Vec2 b, Vec2 c, TextureData texture, bool matchResolution = false);
        void DrawRect(Vec2 a, Vec2 b, TextureData texture, bool matchResolution = false);
        void DrawText(Vec2 pos, std::string text, float scale, TextureGroup font);
    };

    void Log(std::string msg);

    //input methods

    Vec2 GetCursorPos();
    bool IsKeyPressed(int key);
    bool IsMouseButtonPressed(int button);

    //camera methods

    void UpdateWorldToNDCMatrix();
    void UpdateNDCToPixelMatrix(unsigned int width, unsigned int height);
    void SetCameraPosition(Vec2 position);
    void SetCameraSize(Vec2 size);
    void SetCameraRotation(float rotation);
    void SetCameraScale(float zoom);

    //layer methods

    void AddLayer(Layer* layer);
    void RemoveLayer(Layer* layer);
    void ClearLayers();

    //texture methods

    std::vector<TextureData> LoadTextures(std::vector<std::filesystem::path> paths);
    TextureGroup LoadBdfFont(std::filesystem::path path);
    std::vector<TextureData> LoadTexturesFromPath(std::filesystem::path path);
    TextureData ResolveTexture(std::filesystem::path path);
    TextureGroup ResolveFont(std::filesystem::path path);

    //scaling methods

    Vec2 SizeToPixels(Vec2 size, bool inWorld);
    Vec2 SizeFromPixels(Vec2 size, bool inWorld);

    //window methods

    std::tuple<unsigned int, unsigned int> GetWindowSize();
    void SetWindowSize(unsigned int width, unsigned int height);
    void SetWindowFullscreen(bool fullscreen);
    void SetWindowName(std::string name);

    //init, update

    void Initialize(int windowWidth, int windowHeight, std::string windowName, bool fullscreen);
    void UpdateLoop();

    //globals

    inline GLFWwindow* Window;

    //vertex array object, vertex buffer object
    inline unsigned int Vao, Vbo, VboCopy;

    //shader uniform handles
    inline unsigned int UniformNdcMatrix, UniformDrawingDepth, UniformTextureDataArray;

    //camera data
    inline Vec2 CameraPosition; //camera's center
    inline Vec2 CameraSize = Vec2(1);
    inline float CameraRotation;
    inline float CameraScale = 1;

    //coordinate transformation matrices
    inline Mat3 WorldToNDCMatrix;
    inline Mat3 NDCToPixelMatrix;

    //layers
    inline std::vector<Ogl::Layer*> Layers;
    inline size_t VideoMemoryUsed; //by the VBO only

    //texture data
    inline unsigned int Atlas; //opengl texture id
    inline RectanglePacker AtlasPacker;
    inline unsigned int AtlasWidth, AtlasHeight;
    inline unsigned char* AtlasData;
    inline std::vector<Ogl::TextureData> Textures;
    inline std::vector<Ogl::TextureGroup> Fonts;
    inline float TextureDataArray[TEXTURE_MAX * 4]; //texture positions and sizes relative to atlas (for shaders)
}
