#include <cstddef>
#include <cstdlib>

#include <iostream>
#include <format>
#include <algorithm>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include "mat3.hpp"
#include "shaders.hpp"
#include "ogl.hpp"

void Ogl::Log(std::string msg)
{
    #ifdef DEBUG_OUTPUT
    std::cout << msg;
    #endif
}

//callbacks

#ifdef DEBUG_OUTPUT
void GlfwErrorCallback(int code, const char* desc)
{
    Ogl::Log(std::format("GLFW error {} - '{}'\n", code, desc));
}

void GLAPIENTRY GlMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* param)
{
    std::string severityStr;
    switch(severity)
    {
        case GL_DEBUG_SEVERITY_HIGH_ARB:
            severityStr = "high";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM_ARB:
            severityStr = "medium";
            break;
        case GL_DEBUG_SEVERITY_LOW_ARB:
            severityStr = "low";
            break;
        default:
            severityStr = "unknown";
            break;
    }

    Ogl::Log(std::format("GL debug - {} severity: '{}'\n", severityStr, message));
}
#endif

void GlfwFramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    Ogl::UpdateNDCToPixelMatrix(width, height);
    Ogl::Invoke<Ogl::WindowResizeEvent>({ width, height });
}

void GlfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Ogl::Invoke<Ogl::KeyPressEvent>({ key, scancode, action, mods });
}

void GlfwCharCallback(GLFWwindow* window, unsigned int codepoint)
{
    Ogl::Invoke<Ogl::CharacterEvent>({ .Utf32 = codepoint });
}

void GlfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Ogl::Invoke<Ogl::MousePressEvent>({ button, action, mods });
}

void GlfwScrollCallback(GLFWwindow* window, double offsetX, double offsetY)
{
    Ogl::ScrollEvent ev = { offsetX, offsetY };
    Ogl::Invoke<Ogl::ScrollEvent>(ev);
}

//buffer operations

//gets next free block of memory in the VBO
std::tuple<size_t, size_t> GetFreeBlock()
{
    size_t offset, size;

    if (Ogl::Layers.size() == 0)
    {
        offset = 0;
        size = BUFFER_SIZE;
    }
    else
    {
        Ogl::Layer* layer = Ogl::Layers.back();
        offset = layer->BlockOffset + layer->BlockSize;
        size = BUFFER_SIZE - offset;
    }

    return { offset, size };
}

//changes size of the specified block in VBO
//all of the following blocks are shifted
void ResizeBlock(size_t offset, size_t size, size_t newSize)
{
    Ogl::VideoMemoryUsed += newSize - size;
    if (Ogl::VideoMemoryUsed > BUFFER_SIZE)
        throw std::runtime_error("Out of video memory.");

    //copying everything after the block
    //target buffer enums don't matter, 'GL_COPY_WRITE_BUFFER' is being used both for writing & reading
    size_t copySize = Ogl::VideoMemoryUsed - offset - size;
    glCopyBufferSubData(GL_ARRAY_BUFFER, GL_COPY_WRITE_BUFFER, offset + size, 0, copySize);
    glCopyBufferSubData(GL_COPY_WRITE_BUFFER, GL_ARRAY_BUFFER, 0, offset + newSize, copySize);
}

//removes specified block from the VBO
//all of the following blocks are shifted
void RemoveBlock(size_t offset, size_t size)
{
    ResizeBlock(offset, size, 0);
}

//changes size of the memory block occupied by the layer + updates offsets of following blocks
void SetLayerSize(Ogl::Layer* layer, size_t size)
{
    ResizeBlock(layer->BlockOffset, layer->BlockSize, size);
    for (Ogl::Layer* ptr : Ogl::Layers)
    {
        if (ptr != layer && ptr->BlockOffset >= layer->BlockOffset)
            ptr->BlockOffset += size - layer->BlockSize;
    }
    layer->BlockSize = size;
}

//layer methods

void Ogl::AddLayer(Layer* layer)
{
    auto[offset, size] = GetFreeBlock();
    layer->BlockOffset = offset;
    Layers.push_back(layer);
}

void Ogl::RemoveLayer(Layer* layer)
{
    RemoveBlock(layer->BlockOffset, layer->BlockSize);
    for (Layer* ptr : Layers)
    {
        if (ptr->BlockOffset > layer->BlockOffset)
            ptr->BlockOffset -= layer->BlockSize;
    }

    Layers.erase(std::find(Layers.begin(), Layers.end(), layer));
    delete layer;
}

void Ogl::ClearLayers()
{
    for (Layer* layer : Layers)
    {
        RemoveLayer(layer);
    }
}

//window methods

//gets size of the window's framebuffer in pixels
std::tuple<unsigned int, unsigned int> Ogl::GetWindowSize()
{
    int width, height;
    glfwGetFramebufferSize(Window, &width, &height);
    return { static_cast<unsigned int>(width), static_cast<unsigned int>(height) };
}

//sets size of the window's content area in screen coordinates, NOT pixels
void Ogl::SetWindowSize(unsigned int width, unsigned int height)
{
    glfwSetWindowSize(Window, width, height);
}

void Ogl::SetWindowFullscreen(bool fullscreen)
{
    int width, height;
    glfwGetWindowSize(Window, &width, &height);
    glfwSetWindowMonitor(Window, fullscreen ? glfwGetPrimaryMonitor() : NULL, 0, 0, width, height, GLFW_DONT_CARE);
}

void Ogl::SetWindowName(std::string name)
{
    glfwSetWindowTitle(Window, name.c_str());
}

//init, update

void Ogl::Initialize(int windowWidth, int windowHeight, std::string windowName, bool fullscreen)
{
    //!! window creation !!

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    #ifdef DEBUG_OUTPUT
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
    glfwSetErrorCallback(GlfwErrorCallback);
    #endif

    Window = glfwCreateWindow(windowWidth, windowHeight, windowName.c_str(), fullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);
    if (Window == NULL)
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
    glGenBuffers(1, &Vbo);
    glGenBuffers(1, &VboCopy); //for moving data inside of the VBO, isn't used for drawing

    glBindVertexArray(Vao);

    //allocating storage
    glBindBuffer(GL_ARRAY_BUFFER, Vbo);
    glBufferData(GL_ARRAY_BUFFER, BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_COPY_WRITE_BUFFER, VboCopy);
    glBufferData(GL_COPY_WRITE_BUFFER, BUFFER_SIZE, NULL, GL_DYNAMIC_COPY);

    //vertex attributes, interleaved
    //coords - 2 floats
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, VERT_SIZE, 0);
    glEnableVertexAttribArray(0);

    //texture coords - 2 floats
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, VERT_SIZE, reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //texture handle index - 1 uint
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, VERT_SIZE, reinterpret_cast<void*>(4 * sizeof(float)));
    glEnableVertexAttribArray(2);

    //!! shader compilation !!

    unsigned int vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &VertexShaderSource, NULL);
    glCompileShader(vertShader);

    int success;
    char msg[256];
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertShader, 256, NULL, msg);
        throw std::runtime_error(std::format("Error while compiling the vertex shader: '{}'.", msg));
    }

    unsigned int fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &FragmentShaderSource, NULL);
    glCompileShader(fragShader);

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragShader, 256, NULL, msg);
        throw std::runtime_error(std::format("Error while compiling the fragment shader: '{}'.", msg));
    }

    unsigned int shaders;
    shaders = glCreateProgram();
    glAttachShader(shaders, vertShader);
    glAttachShader(shaders, fragShader);
    glLinkProgram(shaders);

    glGetProgramiv(shaders, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(shaders, 256, NULL, msg);
        throw std::runtime_error(std::format("Error while linking shaders: '{}'.", msg));
    }
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    glUseProgram(shaders);

    //shader uniform values
    UniformNdcMatrix = glGetUniformLocation(shaders, "NDCMatrix");
    UniformDrawingDepth = glGetUniformLocation(shaders, "DrawingDepth");
    UniformTextureDataArray = glGetUniformLocation(shaders, "TextureDataArray");

    //enale depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    glClearDepth(0);

    //enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Ogl::UpdateLoop()
{
    while (!glfwWindowShouldClose(Window)) 
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (int i = 0; i < Layers.size(); i++)
        {
            Layer* layer = Layers[i];
            layer->RenderingDataUsed = 0;
            layer->Draw();

            //substituting buffer's data by layer's newly generated one
            size_t dataSize = layer->RenderingDataUsed;
            if (dataSize > 0 || layer->Redraw)
            {
                if (dataSize > layer->BlockSize)
                {
                    Log(std::format("Layer no. {} has exceeded it's memory limit, expanding from {} to {} bytes.\n", i, layer->BlockSize, dataSize * 2));
                    SetLayerSize(layer, dataSize * 2);
                }

                layer->BlockUsed = dataSize;

                if (dataSize > 0)
                    glBufferSubData(GL_ARRAY_BUFFER, layer->BlockOffset, layer->BlockUsed, layer->RenderingData);

                layer->Redraw = false;
            }

            //setting depth and transform matrix
            glUniform1f(UniformDrawingDepth, static_cast<float>(layer->DrawingDepth) / DEPTH_MAX);
            if (layer->IsWorldSpace)
            {
                glUniformMatrix3fv(UniformNdcMatrix, 1, GL_FALSE, WorldToNDCMatrix.Cells);
            }
            else
            {
                glUniformMatrix3fv(UniformNdcMatrix, 1, GL_FALSE, IDENTITY_MATRIX.Cells);
            }

            //draw call
            glDrawArrays(GL_TRIANGLES, layer->BlockOffset / VERT_SIZE, layer->BlockUsed / VERT_SIZE);
        }

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }
}
