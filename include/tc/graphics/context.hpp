#pragma once

#include <filesystem>
#include "tc/misc/mat3.hpp"
#include "buffer.hpp"
#include "layer.hpp"
#include "rectangle_packer.hpp"
#include "texture.hpp"

#define IMAGE_CHANNELS 4 //rgba, just to avoid magic numbers
#define BUFFER_SIZE (VERT_SIZE * 3 * 1000000) //68.6 Mbs, up to a million triangles
#define IMAGE_EXTS { ".png", ".jpeg", ".bmp" }
#define SSBO_BINDING 1

namespace Tc::Graphics
{
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

    //layer methods

    void AddLayer(Layer* layerPtr);
    void RemoveLayer(Layer* layerPtr);
    void SetLayerHeight(Layer* layerPtr, unsigned int height);
    void ClearLayers();
    bool IsLayerOutOfView(Layer* layerPtr);

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

    //init, update

    void Initialize(int windowWidth, int windowHeight, std::string windowName, bool fullscreen = false, bool resizable = true);
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
