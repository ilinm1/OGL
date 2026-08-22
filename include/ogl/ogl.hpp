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
#include "events.hpp"
#include "rectangle_packer.hpp"

#define IMAGE_CHANNELS 4 //rgba, just to avoid magic numbers
#define VERT_SIZE (4 * sizeof(float) + 2 * sizeof(unsigned int))
#define BUFFER_SIZE (VERT_SIZE * 3 * 1000000) //68.6 Mbs, up to a million triangles

#define IMAGE_EXTS { ".png", ".jpeg", ".bmp" }

#define HEIGHT_MAX 0xFFFFFFFF
#define HEIGHT_MIN 0

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

    //rendering layer, each layer owns a block of video memory
    struct Layer : Ogl::Subscriber
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

        Layer(
            bool isWorldSpace = false,
            unsigned int primitiveType = GL_TRIANGLES,
            unsigned int drawingHeight = HEIGHT_MIN,
            size_t renderingDataSize = 256) :
            IsWorldSpace(isWorldSpace),
            PrimitiveType(primitiveType),
            DrawingHeight(drawingHeight),
            RenderingDataSize(renderingDataSize),
            RenderingData(new char[RenderingDataSize]) {}

        //each draw call generates new primitives to be drawn, replacing the old ones; if no new ones were generated the old ones will be drawn
        virtual void Draw() {}

        virtual ~Layer()
        {
            delete[] RenderingData;
        }

        void WriteVertexData(const Vec2* coords, const Vec2* texCoords, const Color* colors, Texture texture, size_t count);
        void DrawTriangle(Vec2 a, Vec2 b, Vec2 c, Color color = COLOR_TRANSPARENT, Texture texture = Texture{}, bool matchResolution = false);
        void DrawRect(Vec2 a, Vec2 b, Color color = COLOR_TRANSPARENT, Texture texture = Texture {}, bool matchResolution = false, bool mirrorX = false, bool mirrorY = false, bool swapXY = false);
        std::vector<Vec2> DrawText(Vec2 pos, std::string text, float scale, BitmapFont& font, Color color = COLOR_TRANSPARENT, bool matchResolution = false, bool multiline = true, bool bounded = false, float maxWidth = 0.0f, float maxHeight = 0.0f);
        std::vector<Vec2> DrawText(Vec2 pos, std::basic_string<unsigned int> text, float scale, BitmapFont& font, Color color = COLOR_TRANSPARENT, bool matchResolution = false, bool multiline = true, bool bounded = false, float maxWidth = 0.0f, float maxHeight = 0.0f);
        void DrawLine(Vec2 a, Vec2 b, Color color);
    };

    //widgets

    namespace Widgets
    {
        struct WidgetLayer;

        struct Widget : Subscriber
        {
            Vec2 Position;
            Vec2 Dimensions;
            WidgetLayer* Parent = nullptr;

            Widget() : Position(0), Dimensions(0) {}

            Widget(Vec2 position, Vec2 dimensions)
            {
                Position = position;
                Dimensions = dimensions;
            }

            virtual void Draw() {}
        };

        struct WidgetLayer : Layer
        {
            std::vector<Widget*> Widgets;

            WidgetLayer(
                bool isWorldSpace = false,
                unsigned int primitiveType = GL_TRIANGLES,
                unsigned int drawingHeight = HEIGHT_MIN,
                size_t renderingDataSize = 256) :
                Layer(isWorldSpace, primitiveType, drawingHeight, renderingDataSize) {}

            void AddWidget(Widget* widgetPtr)
            {
                widgetPtr->Parent = this;
                Widgets.push_back(widgetPtr);
            }

            void RemoveWidget(Widget* widgetPtr)
            {
                widgetPtr->Parent = nullptr;
                Widgets.erase(std::find(Widgets.begin(), Widgets.end(), widgetPtr));
            }

            void Draw() override
            {
                for (Widget* widget : Widgets)
                {
                    widget->Draw();
                }
            }
        };
    }

    void Log(std::string msg);

    //window methods

    void SetWindowName(std::string name);
    void SetWindowSize(int width, int height);
    std::tuple<int, int> GetWindowSize();
    void SetWindowFullscreen(bool fullscreen);

    //input methods

    Vec2 GetCursorPos();
    bool IsKeyPressed(int key);
    bool IsMouseButtonPressed(int button);
    std::string GetClipboardContents();
    bool OpenFilePicker(std::string title, bool write, std::filesystem::path& path);

    //camera methods

    void UpdateWorldToNDCMatrix();
    void UpdateNDCToPixelMatrix(unsigned int width, unsigned int height);
    void SetCameraPosition(Vec2 position);
    void SetCameraSize(Vec2 size);
    void SetCameraRotation(float rotation);
    void SetCameraScale(float zoom);
    Vec2 PointToPixels(Vec2 point, bool inWorld);
    Vec2 PointFromPixels(Vec2 point, bool inWorld);
    Vec2 SizeToPixels(Vec2 size, bool inWorld);
    Vec2 SizeFromPixels(Vec2 size, bool inWorld);

    //texture methods

    void SetTextureFilter(unsigned int minification, unsigned int magnification);
    std::vector<Texture> LoadTextures(std::vector<std::filesystem::path> paths);
    BitmapFont& LoadBdfFont(std::filesystem::path path);
    std::vector<Texture> LoadTexturesFromPath(std::filesystem::path path);
    Texture ResolveTexture(std::filesystem::path path);
    BitmapFont& ResolveFont(std::filesystem::path path);
    void SaveAtlas(std::filesystem::path path);

    //layer methods

    void AddLayer(Layer* layerPtr);
    void RemoveLayer(Layer* layerPtr);
    void SetLayerHeight(Layer* layerPtr, unsigned int height);
    void ClearLayers();
    bool IsLayerOutOfView(Layer* layerPtr);

    //init, update

    void Initialize(int windowWidth, int windowHeight, std::string windowName = "OGL", bool fullscreen = false, bool resizable = true);
    void UpdateLoop();

    //globals

    inline GLFWwindow* Window;

    //vertex array object
    inline unsigned int Vao;

    //vertex buffer object, vertex buffer copy, shader storage buffer object
    inline Buffer Vbo, VboCopy, Ssbo;

    //shader uniform handles
    inline unsigned int UniformNdcMatrix;

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
    inline std::vector<unsigned int> TexturesToUpdate; //indices of newly added/moved textures which require their data to be resent to the GPU

    //layers
    inline unsigned int LastLayerId = 0; //for logging purpouses, so that each layer has a unique ID
    inline std::vector<Layer*> Layers;
}
