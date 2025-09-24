#pragma once

#include <cstddef>
#include <string>
#include <filesystem>
#include <unordered_set>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "vec2.hpp"
#include "mat3.hpp"
#include "rectangle_packer.hpp"

#define IMAGE_CHANNELS 4 //rgba, just to avoid magic numbers
#define VERT_SIZE (4 * sizeof(float) + sizeof(unsigned int))
#define BUFFER_SIZE (VERT_SIZE * 3 * 100000)

#define IMAGE_EXTS { ".png", ".jpeg", ".bmp" }

//real depth is a float, equal to layer's depth divided by this value
//that's the maximal value for 16 bit depth buffer, though 24 and 32 bit buffers may support even more different depths
#define DEPTH_MAX 65535
#define DEPTH_MIN 0

#define SSBO_BINDING 1

namespace Ogl
{
    //block of video memory inside of a buffer object
    struct BufferBlock
    {
        unsigned int Offset = 0;
        unsigned int Size = 0;
        unsigned int Used = 0;
    };

    //wrapper around opengl's buffer
    struct Buffer
    {
        unsigned int Name = 0;
        unsigned int CopyName = 0;
        unsigned int Size = 0;

        unsigned int Usage = GL_DYNAMIC_DRAW;
        unsigned int Binding = 0;

        std::vector<BufferBlock> Blocks;

        //not doing all of this in constructor since glad must be initialized beforehand
        void Initialize(unsigned int name,
                        unsigned int copyName,
                        unsigned int size,
                        unsigned int usage,
                        unsigned int binding)
        {
            if (name == 0)
                glGenBuffers(1, &Name);

            CopyName = copyName;
            Size = size;
            Usage = usage;
            Binding = binding;

            glBindBuffer(binding, Name);
            glBufferData(binding, size, NULL, Usage);
        }

        size_t AddBlock(unsigned int size = 0)
        {
            BufferBlock block;
            if (Blocks.empty())
            {
                block = { 0, size };
            }
            else
            {
                BufferBlock lastBlock = Blocks.back();
                block = { lastBlock.Offset + lastBlock.Size, size };
            }

            Blocks.push_back(block);
            ResizeBlock(Blocks.size() - 1, size);
            return Blocks.size() - 1;
        }

        void ResizeBlock(size_t index, unsigned int size)
        {
            BufferBlock& block = Blocks[index];

            if (block.Size == size)
                return;

            unsigned int used = Blocks.back().Offset + Blocks.back().Size;
            if (used + size - block.Size > Size)
                throw std::runtime_error("Out of video memory.");

            //copying data after the block to it's new end position
            unsigned int copySize = used - block.Offset - block.Size;
            glBindBuffer(GL_COPY_READ_BUFFER, Name);
            glBindBuffer(GL_COPY_WRITE_BUFFER, CopyName);
            glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, block.Offset + block.Size, 0, copySize);
            glCopyBufferSubData(GL_COPY_WRITE_BUFFER, GL_COPY_READ_BUFFER, 0, block.Offset + size, copySize);

            for (int i = index + 1; i < Blocks.size(); i++)
            {
                Blocks[i].Offset += size - block.Size;
            }

            block.Size = size;
        }

        void RemoveBlock(size_t index)
        {
            ResizeBlock(index, 0);
            Blocks.erase(Blocks.begin() + index);
        }
    };

    //input events

    template <class T>
    using EventHandler = void(*)(T, void*);

    template <class T>
    struct Subscription
    {
        EventHandler<T> Handler;
        void* Data;

        friend bool operator==(const Subscription<T>& l, const Subscription<T>& r)
        {
            return l.Handler == r.Handler && l.Data == r.Data;
        }
    };

    template <class T>
    struct std::hash<Ogl::Subscription<T>>
    {
        std::size_t operator()(const Ogl::Subscription<T>& sub) const noexcept
        {
            return ((std::hash<void*>()(sub.Handler) ^ (std::hash<void*>()(sub.Data) << 1)) >> 1);
        }
    };

    struct WindowResizeEvent
    {
        int Width;
        int Height;
    };

    struct KeyPressEvent
    {
        int Key;
        int Scancode;
        int Action;
        int Modifiers;
    };

    struct CharacterEvent
    {
        unsigned int Codepoint; //utf32 codepoint
    };

    struct MousePressEvent
    {
        int Button;
        int Action;
        int Modifiers;
    };

    struct ScrollEvent
    {
        double OffsetX;
        double OffsetY;
    };

    //texture data

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
        size_t Index = 0; //index in 'Textures' and 'TextureDimensionsVector'
    };

    struct BitmapFont
    {
        std::filesystem::path Path;
        unsigned int MaxWidth;
        unsigned int MaxHeight;

        size_t GlyphCount = 0;
        std::vector<std::tuple<unsigned int, unsigned int, size_t>> EncodingRanges; //first utf32 codepoint, second codepoint, first glyph index
    };

    //rendering layer, each layer owns a block of video memory which it can edit in order to draw stuff (i.e. background, menu, terrain, objects, etc.)
    //drawing order is random but drawing depth allows to specify which layers should be on top of others (higher depth layers are drawn ontop of lower ones)
    struct Layer
    {
        size_t BlockIndex; //index of the block of video memory owned by this layer
        size_t Index = 0;

        unsigned int DrawingDepth = DEPTH_MIN;
        bool IsWorldSpace = false;
        bool Redraw = false; //if set data from the previous 'Draw' call will be discarded even if nothing was generated during the last call; will be reset afterwards

        size_t RenderingDataSize = 0;
        size_t RenderingDataUsed = 0;
        char* RenderingData = NULL;

        Layer(size_t renderingDataSize = 256)
        {
            RenderingData = new char[renderingDataSize];
            RenderingDataSize = renderingDataSize;
        }

        virtual void Draw() {}

        virtual ~Layer() 
        {
            delete[] RenderingData;
        }

        //drawing methods

        void WriteVertexData(const Vec2* coords, const Vec2* texCoords, Texture texture, size_t count);
        void DrawTriangle(Vec2, Vec2 b, Vec2 c, Texture texture, bool matchResolution = false);
        void DrawRect(Vec2 a, Vec2 b, Texture texture, bool matchResolution = false);
        void DrawText(Vec2 pos, std::string text, float scale, BitmapFont& font, bool multiline = true);
    };

    void Log(std::string msg);

    //window methods

    std::tuple<int, int> GetWindowSize();
    void SetWindowSize(int width, int height);
    void SetWindowFullscreen(bool fullscreen);
    void SetWindowName(std::string name);

    //input methods

    Vec2 GetCursorPos();
    bool IsKeyPressed(int key);
    bool IsMouseButtonPressed(int button);
    std::string GetClipboardContents();

    //input event method implementations - since they depend on types from this header
    //and templates must be defined in headers it's easier to just put them here

    template <class T>
    std::unordered_set<Subscription<T>>& GetSubscriptions()
    {
        static std::unordered_set<Subscription<T>> subs;
        return subs;
    }

    template <class T>
    void Subscribe(EventHandler<T> handler, void* data = NULL)
    {
        GetSubscriptions<T>().insert({ handler, data });
    }

    template <class T>
    void Unsubscribe(EventHandler<T> handler, void* data = NULL)
    {
        GetSubscriptions<T>().erase({ handler, data });
    }

    template <class T>
    void Invoke(T event)
    {
        for (Subscription sub : GetSubscriptions<T>())
        {
            sub.Handler(event, sub.Data);
        }
    }

    //camera methods

    void UpdateWorldToNDCMatrix();
    void UpdateNDCToPixelMatrix(unsigned int width, unsigned int height);
    void SetCameraPosition(Vec2 position);
    void SetCameraSize(Vec2 size);
    void SetCameraRotation(float rotation);
    void SetCameraScale(float zoom);
    Vec2 PointToPixels(Vec2 point, bool inWorld);
    Vec2 PointFromPixels(Vec2 point, bool inWorld);

    //texture methods

    std::vector<Texture> LoadTextures(std::vector<std::filesystem::path> paths);
    BitmapFont LoadBdfFont(std::filesystem::path path);
    std::vector<Texture> LoadTexturesFromPath(std::filesystem::path path);
    Texture ResolveTexture(std::filesystem::path path);
    BitmapFont ResolveFont(std::filesystem::path path);

    //layer methods

    void AddLayer(Layer* layer);
    void RemoveLayer(Layer* layer);
    void ClearLayers();

    //init, update

    void Initialize(int windowWidth, int windowHeight, std::string windowName, bool fullscreen);
    void UpdateLoop();

    //globals

    inline GLFWwindow* Window;

    //vertex array object
    inline unsigned int Vao;

    //vertex buffer object, vertex buffer copy, shader storage buffer object
    inline Buffer Vbo, VboCopy, Ssbo;

    //shader uniform handles
    inline unsigned int UniformNdcMatrix, UniformDrawingDepth;

    //camera data
    inline Vec2 CameraPosition; //camera's center
    inline Vec2 CameraSize = Vec2(1);
    inline float CameraRotation;
    inline float CameraScale = 1;

    //coordinate transformation matrices
    inline Mat3 WorldToNDCMatrix;
    inline Mat3 NDCToPixelMatrix;

    //texture data
    inline unsigned int Atlas; //opengl texture id
    inline RectanglePacker AtlasPacker;
    inline unsigned int AtlasWidth, AtlasHeight;
    inline unsigned char* AtlasData;
    inline std::vector<Texture> Textures;
    inline std::vector<BitmapFont> Fonts;
    inline std::vector<TextureDimensions> TextureDimensionsVector; //texture positions and sizes relative to atlas, storing them separately from other texture data since it must be sent to the fragment shader
    inline std::vector<size_t> TexturesToUpdate; //indices of newly added/moved textures which require their data to be resent to the GPU

    //layers
    inline std::vector<Layer*> Layers;
}
