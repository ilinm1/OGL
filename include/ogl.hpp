#pragma once

#include <string>
#include <filesystem>
#include <set>
#include <functional>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "vec2.hpp"
#include "mat3.hpp"
#include "color.hpp"
#include "rectangle_packer.hpp"

#define IMAGE_CHANNELS 4 //rgba, just to avoid magic numbers
#define VERT_SIZE (4 * sizeof(float) + 2 * sizeof(unsigned int))
#define BUFFER_SIZE (VERT_SIZE * 3 * 1000000) //68.6 Mbs, up to a million triangles

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

        void Initialize(unsigned int name, unsigned int copyName, unsigned int size, unsigned int usage, unsigned int binding);
        size_t AddBlock(unsigned int size = 0);
        void ResizeBlock(size_t index, unsigned int size);
        void RemoveBlock(size_t index);
    };

    //input events

    template <class T>
    using EventHandler = void(*)(T, void*, bool&); //event, data, whether event has been handled

    template <class T>
    struct Subscription
    {
        EventHandler<T> Handler;
        int Priority; //handlers with higher priority will be called first
        void* Data;

        friend bool operator==(const Subscription<T>& l, const Subscription<T>& r)
        {
            return l.Handler == r.Handler && l.Data == r.Data;
        }

        bool operator>(const Subscription<T>& x) const
        {
            return Priority > x.Priority;
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

    template <class T>
    using SubscriptionSet = std::set<Subscription<T>, std::greater<Subscription<T>>>;

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
        size_t Index = 0; //index in 'Textures' and 'TextureDimensionsVector'; if index is zero then texture is invalid
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

        unsigned int PrimitiveType = GL_TRIANGLES; //most drawing methods use GL_TRIANGLES, each layer can only use one primitive per draw call
        unsigned int DrawingDepth = DEPTH_MIN; //layer with higher depth will be drawn on top of layers with lower depth
        bool IsWorldSpace = false; //if set objects drawn by the layer will be transformed to NDC from world coordinates by the vertex shader
        bool Redraw = false; //if set data from the previous 'Draw' call will be discarded even if nothing was generated during the last call; will be reset afterwards
        bool IsOutOfView = false; //if set layer is currently out of view and won't be drawn

        Vec2 AabbMax = Vec2(0); //AABB of objects drawn by the layer, used for clipping (if enabled), WILL NOT BE SET WHEN USING 'WriteVertexData' DIRECTLY
        Vec2 AabbMin = Vec2(0);

        size_t RenderingDataSize = 0;
        size_t RenderingDataUsed = 0;
        char* RenderingData = NULL;

        std::vector<std::function<void()>> UnsubHandlers; //lambdas wrapped in 'std::function' which will be called upon layer's destruction to unsubscribe it from any events

        Layer(size_t renderingDataSize = 256)
        {
            RenderingData = new char[renderingDataSize];
            RenderingDataSize = renderingDataSize;
        }

        //each draw call generates new primitives to be drawn, replacing the old ones; if no new ones were generated the old ones will be drawn
        virtual void Draw() {}

        virtual ~Layer()
        {
            for (std::function<void()> handler : UnsubHandlers)
            {
                handler();
            }
            delete[] RenderingData;
        }

        //drawing methods

        void WriteVertexData(const Vec2* coords, const Vec2* texCoords, const Color* colors, Texture texture, size_t count);
        void DrawTriangle(Vec2 a, Vec2 b, Vec2 c, Color color = COLOR_TRANSPARENT, Texture texture = Texture{}, bool matchResolution = false);
        void DrawRect(Vec2 a, Vec2 b, Color color = COLOR_TRANSPARENT, Texture texture = Texture {}, bool matchResolution = false, bool mirrorX = false, bool mirrorY = false, bool swapXY = false);
        void DrawText(Vec2 pos, std::string text, float scale, BitmapFont& font, Color color = COLOR_TRANSPARENT, bool multiline = true, bool bounded = false, float maxWidth = 0.0f, float maxHeight = 0.0f);
        void DrawLine(Vec2 a, Vec2 b, Color color);
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
    bool OpenFilePicker(std::string title, bool write, std::filesystem::path& path);

    //input event method implementations - since they depend on types from this header
    //and templates must be defined in headers it's easier to just put them here

    template <class T>
    SubscriptionSet<T>& GetSubscriptions()
    {
        static SubscriptionSet<T> subs;
        return subs;
    }

    template <class T>
    void Unsubscribe(const Subscription<T>& sub)
    {
        GetSubscriptions<T>().erase(sub);
    }

    //subscribes 'handler' to the event T
    //if 'layer' is set an unsubscription handler will be created and called automatically upon layer's destruction, otherwise 'Unsubscribe' should be called manually
    //if 'data' is not null it will be passed to the handler, otherwise 'layer' will be passed instead
    //handlers with higher 'priority' will get called first
    template <class T>
    const Subscription<T>& Subscribe(EventHandler<T> handler, Ogl::Layer* layer = NULL, void* data = NULL, int priority = 0)
    {
        const Subscription<T>& sub = *(GetSubscriptions<T>().insert({ handler, priority, data ? data : layer }).first);
        if (layer)
            layer->UnsubHandlers.push_back(std::function<void()>([sub]() { Ogl::Unsubscribe(sub); }));
        return sub;
    }

    template <class T>
    void Invoke(T event)
    {
        bool handled = false;
        for (Subscription sub : GetSubscriptions<T>())
        {
            sub.Handler(event, sub.Data, handled);
            if (handled)
                return;
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

    void SetTextureFilter(unsigned int minification, unsigned int magnification);
    std::vector<Texture> LoadTextures(std::vector<std::filesystem::path> paths);
    BitmapFont LoadBdfFont(std::filesystem::path path);
    std::vector<Texture> LoadTexturesFromPath(std::filesystem::path path);
    Texture ResolveTexture(std::filesystem::path path);
    BitmapFont ResolveFont(std::filesystem::path path);

    //layer methods

    void AddLayer(Layer* layer);
    void RemoveLayer(Layer* layer);
    void ClearLayers();
    bool IsLayerOutOfView(Layer* layer);

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
    inline Vec2 CameraSize = Vec2(1); //two times the distance from the camera's center to it's x/y boundary
    inline float CameraRotation;
    inline float CameraScale = 1;
    inline bool ClippingEnabled = true; //if enabled, layers which are out of camera's view will not be drawn

    //coordinate transformation matrices
    inline Mat3 WorldToNDCMatrix;
    inline Mat3 NDCToWorldMatrix;
    inline Mat3 NDCToPixelMatrix;
    inline Mat3 PixelToNDCMatrix;

    //texture data
    inline unsigned int Atlas; //opengl texture id
    inline RectanglePacker AtlasPacker;
    inline unsigned int AtlasWidth, AtlasHeight;
    inline unsigned char* AtlasData;
    inline std::vector<Texture> Textures = { Texture {} }; //zero index is reserved as an invalid texture, so drawing commands will ignore it
    inline std::vector<BitmapFont> Fonts;
    inline std::vector<TextureDimensions> TextureDimensionsVector = { TextureDimensions {} }; //texture positions and sizes relative to atlas, storing them separately from other texture data since it must be sent to the fragment shader
    inline std::vector<size_t> TexturesToUpdate; //indices of newly added/moved textures which require their data to be resent to the GPU

    //layers
    inline std::vector<Layer*> Layers;
}
