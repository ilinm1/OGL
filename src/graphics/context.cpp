#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <format>
#include <algorithm>
#include <vector>
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "tc/misc/events.hpp"
#include "tc/misc/mat3.hpp"
#include "tc/misc/misc.hpp"
#include "tc/misc/vec2.hpp"
#include "tc/graphics/buffer.hpp"
#include "tc/graphics/context.hpp"
#include "tc/graphics/input_events.hpp"
#include "tc/graphics/layer.hpp"
#include "tc/graphics/shaders.hpp"

namespace Tcg = Tc::Graphics;

//callbacks

#ifdef DEBUG_OUTPUT
void GlfwErrorCallback(int code, const char* desc)
{
    Tc::Log(std::format("GLFW error {} - '{}'\n", code, desc));
}

void GLAPIENTRY GlMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* param)
{
    std::string severityStr;
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
        severityStr = "high severity";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        severityStr = "medium severity";
        break;
    case GL_DEBUG_SEVERITY_LOW:
        severityStr = "low severity";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        severityStr = "notification";
        break;
    default:
        severityStr = "unknown";
        break;
    }

    Tc::Log(std::format("GL debug - {}: '{}'\n", severityStr, message));
}
#endif

void GlfwFramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    Tcg::UpdateNDCToPixelMatrix(width, height);
    Tcg::WindowResizeEvent ev = { width, height };
    Tc::Invoke<Tcg::WindowResizeEvent>(ev);
}

void GlfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Tcg::KeyPressEvent ev = { key, scancode, action, mods };
    Tc::Invoke<Tcg::KeyPressEvent>(ev);
}

void GlfwCharCallback(GLFWwindow* window, unsigned int codepoint)
{
    Tcg::CharacterEvent ev = { .Codepoint = codepoint };
    Tc::Invoke<Tcg::CharacterEvent>(ev);
}

void GlfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Tcg::MousePressEvent ev = { button, action, mods };
    Tc::Invoke<Tcg::MousePressEvent>(ev);
}

void GlfwScrollCallback(GLFWwindow* window, double offsetX, double offsetY)
{
    Tcg::ScrollEvent ev = { offsetX, offsetY };
    Tc::Invoke<Tcg::ScrollEvent>(ev);
}

//window methods

void Tcg::SetWindowName(std::string name)
{
    glfwSetWindowTitle(Window, name.c_str());
}

//sets size of the window's content area in screen coordinates (generally this size doesn't have to be the same as size of the window in pixels but it usually is)
void Tcg::SetWindowSize(int width, int height)
{
    glfwSetWindowSize(Window, width, height);
}

//gets size of the window's framebuffer in pixels
std::tuple<int, int> Tcg::GetWindowSize()
{
    int width, height;
    glfwGetFramebufferSize(Window, &width, &height);
    return { width, height };
}

void Tcg::SetWindowFullscreen(bool fullscreen)
{
    int width, height;
    glfwGetWindowSize(Window, &width, &height);
    glfwSetWindowMonitor(Window, fullscreen ? glfwGetPrimaryMonitor() : nullptr, 0, 0, width, height, GLFW_DONT_CARE);
}

//input methods

Tc::Vec2 Tcg::GetCursorPos()
{
    double cx, cy;
    glfwGetCursorPos(Window, &cx, &cy);
    return Vec2(static_cast<float>(cx), static_cast<float>(cy));
}

//returns true if key's last reported state is 'GLFW_PRESS'
//key is one of GLFW's key IDs
bool Tcg::IsKeyPressed(int key)
{
    return glfwGetKey(Window, key) == GLFW_PRESS;
}

bool Tcg::IsMouseButtonPressed(int button)
{
    return glfwGetMouseButton(Window, button) == GLFW_PRESS;
}

std::string Tcg::GetClipboardContents()
{
    const char* ptr = glfwGetClipboardString(nullptr);
    if (ptr == nullptr)
        return "";
    return std::string(ptr);
}

//layer methods

bool CompareLayersByHeight(Tcg::Layer*& a, Tcg::Layer*& b)
{
    return a->DrawingHeight < b->DrawingHeight;
}

void SortLayersByHeight()
{
    std::sort(Tcg::Layers.begin(), Tcg::Layers.end(), CompareLayersByHeight);
}

void Tcg::SetLayerHeight(Tcg::Layer* layerPtr, unsigned int height)
{
    layerPtr->DrawingHeight = height;
    SortLayersByHeight();
}

void Tcg::AddLayer(Tcg::Layer* layerPtr)
{
    layerPtr->Id = Tcg::LastLayerId++;
    layerPtr->BlockIndex = Vbo.AddBlock();
    Layers.push_back(layerPtr);
    SortLayersByHeight();
    Log(std::format("Added layer no. {}.\n", layerPtr->Id));
}

void Tcg::RemoveLayer(Tcg::Layer* layerPtr)
{
    Log(std::format("Removing layer no. {}.\n", layerPtr->Id));

    Vbo.RemoveBlock(layerPtr->BlockIndex);
    for (Tcg::Layer* layer : Layers)
    {
        if (layer->BlockIndex > layerPtr->BlockIndex)
            layer->BlockIndex--;
    }

    Layers.erase(std::find(Layers.begin(), Layers.end(), layerPtr));
}

void Tcg::ClearLayers()
{
    for (Tcg::Layer* layer : Layers)
    {
        RemoveLayer(layer);
    }
}

bool Tcg::IsLayerOutOfView(Tcg::Layer* layerPtr)
{
    if (layerPtr->IsWorldSpace)
    {
        Vec2 cameraMax = CameraPosition + CameraSize * CameraScale;
        Vec2 cameraMin = CameraPosition - CameraSize * CameraScale;
        return (layerPtr->AabbMax.X < cameraMin.X || layerPtr->AabbMax.Y < cameraMin.Y) || (layerPtr->AabbMin.X > cameraMax.X || layerPtr->AabbMin.Y > cameraMax.Y);
    }
    else
    {
        return (layerPtr->AabbMax.X < -1 || layerPtr->AabbMax.Y < -1) || (layerPtr->AabbMin.X > 1 || layerPtr->AabbMin.Y > 1);
    }
}

//opens a file picker window with the title 'title'
//if the purpouse of the picker is to select the file for writing then 'write' may be set
//in which case various additional settings will be applied (ovewrite warnings, ignoring readonly files, etc.)
//if user has chosen a valid file then method will return true and set 'path' reference to the path of the selected file
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
bool Tcg::OpenFilePicker(std::string title, bool write, std::filesystem::path& path)
{
    std::filesystem::path result;
    std::string pathStr = std::string(256, 0);

    //https://learn.microsoft.com/en-us/windows/win32/api/commdlg/ns-commdlg-openfilenamea
    OPENFILENAMEA schizoStruct = {
        sizeof(OPENFILENAMEA),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        0,
        1,
        pathStr.data(),
        pathStr.capacity(),
        nullptr,
        0,
        nullptr,
        title.c_str(),
        OFN_FILEMUSTEXIST | (write ? OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN : 0),
        0,
        0,
        nullptr,
        0,
        nullptr,
        nullptr
    };

    GetOpenFileNameA(&schizoStruct);
    result = std::filesystem::path(pathStr);
    if (std::filesystem::exists(result)) //'exists' should be unnecessary here but I've left it just in case
    {
        path = result;
        return true;
    }
    return false;
}
#else
std::filesystem::path OpenFilePicker(std::string title, bool write, std::filesystem::path& path)
{
    throw std::runtime_error("Not implemented for your OS.");
}
#endif

//init, update

void Tcg::Initialize(int windowWidth, int windowHeight, std::string windowName, bool fullscreen, bool resizable)
{
    //!! window creation !!

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, resizable);

#ifdef DEBUG_OUTPUT
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
    glfwSetErrorCallback(GlfwErrorCallback);
#endif

    Window = glfwCreateWindow(windowWidth, windowHeight, windowName.c_str(), fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
    if (Window == nullptr)
        throw std::runtime_error("Failed to create GLFW window.");
    glfwMakeContextCurrent(Window);

    //setting callbacks
    glfwSetFramebufferSizeCallback(Window, GlfwFramebufferSizeCallback);
    glfwSetKeyCallback(Window, GlfwKeyCallback);
    glfwSetCharCallback(Window, GlfwCharCallback);
    glfwSetMouseButtonCallback(Window, GlfwMouseButtonCallback);
    glfwSetScrollCallback(Window, GlfwScrollCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        throw std::runtime_error("Failed to initialize GLAD.");

    //gl debug
#ifdef DEBUG_OUTPUT
    Log(std::format("OpenGL version: {}\n", std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)))));
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallbackARB(GlMessageCallback, 0);
    Log("Using OpenGL debug output.\n");
#endif

    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0, 0, 0, 1);

    UpdateWorldToNDCMatrix();
    UpdateNDCToPixelMatrix(windowWidth, windowHeight);

    //!! buffer generation !!
    //VAO is vertex array object, it holds vertex attributes and a VBO
    //VBO is vertex buffer object, it holds vertex data (each layer has it's own block of memory inside of it)

    glGenVertexArrays(1, &Vao);
    glBindVertexArray(Vao);

    VboCopy.Initialize(0, 0, BUFFER_SIZE, GL_DYNAMIC_COPY, GL_COPY_WRITE_BUFFER);
    Vbo.Initialize(0, VboCopy.Name, BUFFER_SIZE, GL_DYNAMIC_DRAW, GL_ARRAY_BUFFER);
    Ssbo.Initialize(0, 0, BUFFER_SIZE, GL_DYNAMIC_DRAW, GL_SHADER_STORAGE_BUFFER);

    //vertex attributes, interleaved
    //coords - 2 floats
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, VERT_SIZE, 0);
    glEnableVertexAttribArray(0);

    //texture coords - 2 floats
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, VERT_SIZE, reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //texture index - 1 uint
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, VERT_SIZE, reinterpret_cast<void*>(4 * sizeof(float)));
    glEnableVertexAttribArray(2);

    //modulate color - 1 uint
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, VERT_SIZE, reinterpret_cast<void*>(4 * sizeof(float) + sizeof(unsigned int)));
    glEnableVertexAttribArray(3);

    //!! shader compilation !!

    unsigned int vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &VertexShaderSource, nullptr);
    glCompileShader(vertShader);

    int success;
    char msg[256];
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertShader, 256, nullptr, msg);
        throw std::runtime_error(std::format("Error while compiling the vertex shader: '{}'.", msg));
    }

    unsigned int fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &FragmentShaderSource, nullptr);
    glCompileShader(fragShader);

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragShader, 256, nullptr, msg);
        throw std::runtime_error(std::format("Error while compiling the fragment shader: '{}'.", msg));
    }

    unsigned int shaders;
    shaders = glCreateProgram();
    glAttachShader(shaders, vertShader);
    glAttachShader(shaders, fragShader);
    glLinkProgram(shaders);

    glGetProgramiv(shaders, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaders, 256, nullptr, msg);
        throw std::runtime_error(std::format("Error while linking shaders: '{}'.", msg));
    }
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    glUseProgram(shaders);

    //shader uniform values
    UniformNdcMatrix = glGetUniformLocation(shaders, "NDCMatrix");

    //binding ssbo
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_BINDING, Ssbo.Name);

    //enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //querying max texture size
    int maxTexSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
    AtlasPacker.MaxWidth = AtlasPacker.MaxHeight = maxTexSize / 2;
}

void Tcg::UpdateLoop()
{
    while (!glfwWindowShouldClose(Window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        for (Tcg::Layer* layer : Layers)
        {
            Tcg::BufferBlock& layerBlock = Vbo.Blocks[layer->BlockIndex];
            layer->RenderingDataUsed = 0;
            layer->Draw();

            if (ClippingEnabled && IsLayerOutOfView(layer))
            {
                if (!layer->IsOutOfView)
                {
                    Log(std::format("Layer no. {} is out of view and won't be drawn.\n", layer->Id));
                    layer->IsOutOfView = true;
                }

                continue;
            }
            layer->IsOutOfView = false;

            //substituting buffer's data by layer's newly generated one
            size_t dataSize = layer->RenderingDataUsed;
            if (dataSize > 0 || layer->Redraw)
            {
                if (dataSize > layerBlock.Size)
                {
                    Log(std::format("Layer no. {} has exceeded it's memory limit, expanding from {} to {} bytes.\n", layer->Id, layerBlock.Size, dataSize * 2));
                    Vbo.ResizeBlock(layer->BlockIndex, layerBlock.Size * 2 + dataSize);
                }

                layerBlock.Used = dataSize;

                if (dataSize > 0)
                    glBufferSubData(GL_ARRAY_BUFFER, layerBlock.Offset, dataSize, layer->RenderingData);

                layer->Redraw = false;
            }

            //setting transform matrix
            if (layer->IsWorldSpace)
            {
                glUniformMatrix3fv(UniformNdcMatrix, 1, GL_TRUE, WorldToNDCMatrix.Cells);
            }
            else
            {
                glUniformMatrix3fv(UniformNdcMatrix, 1, GL_TRUE, IDENTITY_MATRIX.Cells);
            }

            //draw call
            glDrawArrays(layer->PrimitiveType, layerBlock.Offset / VERT_SIZE, layerBlock.Used / VERT_SIZE);
        }

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }
}


//camera methods

//updates matrix which translates world coordinates to normalized device coordinates, should be called after changing camera parameters
void Tcg::UpdateWorldToNDCMatrix()
{
    float x = CameraPosition.X;
    float y = CameraPosition.Y;
    float w = CameraSize.X * CameraScale / 2.0f;
    float h = CameraSize.Y * CameraScale / 2.0f;
    float c = cosf(CameraRotation);
    float s = sinf(CameraRotation);

    WorldToNDCMatrix =
    {
        c / w, -s / w, (y * s - x * c) / w,
        s / h, c / h, (-y * c - x * s) / h,
        0, 0, 0
    };

    NDCToWorldMatrix =
    {
        w * c, -h * s, x * c - y * s,
        w * s, h * c, x * s + y * c,
        0, 0, 0
    };
}

//updates matrix which translates normalized device coordinates to pixels, should be called after changing window size
void Tcg::UpdateNDCToPixelMatrix(unsigned int width, unsigned int height)
{
    float w = static_cast<float>(width) / 2.0f;
    float h = static_cast<float>(height) / 2.0f;

    NDCToPixelMatrix =
    {
        w, 0, w,
        0, -h, h,
        0, 0, 0
    };

    PixelToNDCMatrix =
    {
        1 / w, 0, -1,
        0, -1 / h, 1,
        0, 0, 0
    };
}

//in in-world meters
void Tcg::SetCameraPosition(Tc::Vec2 position)
{
    CameraPosition = position;
    UpdateWorldToNDCMatrix();
}

//in in-world meters
void Tcg::SetCameraSize(Tc::Vec2 size)
{
    CameraSize = size;
    UpdateWorldToNDCMatrix();
}

//in radians
void Tcg::SetCameraRotation(float rotation)
{
    CameraRotation = rotation;
    UpdateWorldToNDCMatrix();
}

//1 is 100% of normal camera size
void Tcg::SetCameraScale(float zoom)
{
    CameraScale = zoom;
    UpdateWorldToNDCMatrix();
}

//converts a point in NDC/world coordinates to pixels
Tc::Vec2 Tcg::PointToPixels(Vec2 point, bool inWorld)
{
    if (inWorld)
        point = WorldToNDCMatrix.TransformVector(point);
    return NDCToPixelMatrix.TransformVector(point);
}

//converts a point in pixels to NDC/world coordinates
Tc::Vec2 Tcg::PointFromPixels(Vec2 point, bool inWorld)
{
    point = PixelToNDCMatrix.TransformVector(point);
    if (inWorld)
        point = NDCToWorldMatrix.TransformVector(point);
    return point;
}

//unlike 'PointToPixels' doesn't account for camera's position, rotation, different coordinate centers, etc. 
//only converting the actual dimensions of the object
Tc::Vec2 Tcg::SizeToPixels(Vec2 size, bool inWorld)
{
    if (inWorld)
        size = Vec2(size.X / Tcg::CameraSize.X / Tcg::CameraScale, size.Y / Tcg::CameraSize.Y / Tcg::CameraScale);

    auto [width, height] = Tcg::GetWindowSize();
    return Vec2(size.X * width, size.Y * height);
}

//unlike 'PointFromPixels' doesn't account for camera's position, rotation, different coordinate centers, etc. 
//only converting the actual dimensions of the object
Tc::Vec2 Tcg::SizeFromPixels(Vec2 size, bool inWorld)
{
    auto [width, height] = Tcg::GetWindowSize();
    size = Vec2(size.X / width, size.Y / height);

    if (inWorld)
        size = Vec2(size.X * Tcg::CameraSize.X * Tcg::CameraScale, size.Y * Tcg::CameraSize.Y * Tcg::CameraScale);

    return size;
}

//texture methods

//'GL_NEAREST' - no filtering, 'GL_LINEAR' - linear interpolation
void Tcg::SetTextureFilter(unsigned int minification, unsigned int magnification)
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minification);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magnification);
}

Tcg::Texture AddTexture(std::filesystem::path path, Tcg::Rect rect)
{
    Tcg::TextureDimensionsVector.push_back({ rect.X, rect.Y, rect.Width, rect.Height });
    Tcg::TexturesToUpdate.push_back(Tcg::Textures.size());
    Tcg::Textures.push_back({ path, Tcg::Textures.size() });
    return Tcg::Textures.back();
}

//updates texture dimensions stored in the SSBO and resends the entire atlas to the GPU
void UpdateTextureData()
{
    unsigned int maxIndex = *std::max_element(Tcg::TexturesToUpdate.begin(), Tcg::TexturesToUpdate.end());
    unsigned int requiredSsboSize = (maxIndex + 1) * sizeof(Tcg::TextureDimensions);
    if (Tcg::Ssbo.Size < requiredSsboSize)
        throw std::runtime_error("SSBO size exceeded.");

    for (unsigned int index : Tcg::TexturesToUpdate)
    {
        Tcg::TextureDimensions dimensions = Tcg::TextureDimensionsVector[index];
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, index * sizeof(Tcg::TextureDimensions), sizeof(Tcg::TextureDimensions), &dimensions);
    }

    Tcg::TexturesToUpdate.clear();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Tcg::AtlasWidth, Tcg::AtlasHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, Tcg::AtlasData);
}

void InitializeAtlas()
{
    glGenTextures(1, &Tcg::Atlas);
    glBindTexture(GL_TEXTURE_2D, Tcg::Atlas);
    Tcg::SetTextureFilter(GL_NEAREST, GL_NEAREST);
}

//data pointing to the top-left pixel of the image, x & y specifying the left-bottom corner of the image area
//if 'flip' is set the image will be flipped vertically (since opengl treats first pixel as bottom-left loaded images will be displayed upside-down)
void WriteToAtlas(unsigned char* data, unsigned int x, unsigned int y, unsigned int width, unsigned int height, bool flip = true)
{
    if (x + width > Tcg::AtlasWidth || y + height > Tcg::AtlasHeight)
        throw std::runtime_error("Tried to write out of atlas bounds.");

    for (int i = 0; i < height; i++)
    {
        unsigned int dataRow = flip ? height - i - 1 : i;
        std::memcpy(
            Tcg::AtlasData + (Tcg::AtlasWidth * (i + y) + x) * IMAGE_CHANNELS,
            data + width * dataRow * IMAGE_CHANNELS,
            width * IMAGE_CHANNELS);
    }
}

void ResizeAtlas(unsigned int width, unsigned int height)
{
    if (Tcg::AtlasWidth == width && Tcg::AtlasHeight == height)
        return;

    unsigned char* data = new unsigned char[width * height * IMAGE_CHANNELS];

    unsigned char* oldData = Tcg::AtlasData;
    unsigned int oldWidth = Tcg::AtlasWidth;
    unsigned int oldHeight = Tcg::AtlasHeight;

    Tcg::AtlasWidth = width;
    Tcg::AtlasHeight = height;
    Tcg::AtlasData = data;

    if (oldData != nullptr)
    {
        WriteToAtlas(oldData, 0, 0, std::min(oldWidth, width), std::min(oldHeight, height), false);
        delete[] oldData;
    }
}

//loads textures from the specified paths, adding them to atlas
std::vector<Tcg::Texture> Tcg::LoadTextures(std::vector<std::filesystem::path> paths)
{
    if (Atlas == 0)
        InitializeAtlas();

    std::vector<Tcg::Texture> result;

    //getting texture rects
    for (int i = 0; i < paths.size(); i++)
    {
        std::filesystem::path path = paths[i];

        if (!std::filesystem::exists(path))
            throw std::runtime_error(std::format("Invalid texture path: '{}'.", path.string()));

        int width, height, components;
        stbi_info(path.string().c_str(), &width, &height, &components);
        AtlasPacker.Rects.push_back({ .Width = static_cast<unsigned int>(width), .Height = static_cast<unsigned int>(height), .Data = { i, 0 } });
    }

    //packing newly generated rects & resizing the atlas
    AtlasPacker.Pack();
    ResizeAtlas(AtlasPacker.TotalWidth, AtlasPacker.TotalHeight);

    //loading new textures & writing them onto the atlas
    for (Rect rect : AtlasPacker.Rects)
    {
        std::filesystem::path path = paths[get<0>(rect.Data)];

        int _;
        unsigned char* data = stbi_load(path.string().c_str(), &_, &_, &_, IMAGE_CHANNELS);
        if (data == nullptr)
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
Tcg::BitmapFont& Tcg::LoadBdfFont(std::filesystem::path path)
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
    const unsigned int maxOffset = 256; //max x/y offset
    int code, offsetX, offsetY;
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
            sscanf(line.substr(3).data(), "%d %d %d %d", &glyphRect.Width, &glyphRect.Height, &offsetX, &offsetY);

            //todo: BDF format documentation is lacking, after incorrectly trying to implement glyph offsets several times I've decided to cut them out

            if (glyphRect.Width > result.MaxWidth)
                result.MaxWidth = glyphRect.Width;

            if (glyphRect.Height > result.MaxHeight)
                result.MaxHeight = glyphRect.Height;
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

        file.seekg(get<1>(rect.Data)); //going to the start of the bitmap

        unsigned char buffer[IMAGE_CHANNELS * 4]; //4 cause we're writing up to 4 pixels per hexadecimal digit

        for (int y = rect.Height; y > 0; y--)
        {
            std::getline(file, line);

            for (int x = 0; x < line.size(); x++)
            {
                char character = line[x];
                if (character < '0') //ignoring whitespaces
                    continue;

                unsigned char value = character < '9' ? character - '0' : character - 'A' + 10; //hexadecimal digit to 4 bit value

                int pixels = rect.Width - x * 4;
                pixels = std::max(std::min(pixels, 4), 0); //how much pixels we have to actually write

                for (int i = 0; i < pixels; i++)
                {
                    *reinterpret_cast<unsigned int*>(buffer + i * IMAGE_CHANNELS) = value & (0b1000 >> i) ? 0xFFFFFFFF : 0;
                }

                if (pixels != 0)
                    WriteToAtlas(buffer, rect.X + x * 4, rect.Y + y - 1, pixels, 1);
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

    Fonts.push_back(result);
    return Fonts.back();
}

//loads all the textures from the specified path (recursively)
std::vector<Tcg::Texture> Tcg::LoadTexturesFromPath(std::filesystem::path path)
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
Tcg::Texture Tcg::ResolveTexture(std::filesystem::path path)
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
Tcg::BitmapFont& Tcg::ResolveFont(std::filesystem::path path)
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

//writes atlas as a .bmp image (for debugging purpouses)
void Tcg::SaveAtlas(std::filesystem::path path)
{
    stbi_flip_vertically_on_write(true);
    stbi_write_bmp(path.string().c_str(), Tcg::AtlasWidth, Tcg::AtlasHeight, 4, Tcg::AtlasData);
}
