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

#include <mat3.hpp>
#include <shaders.hpp>
#include <ogl.hpp>

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

    Ogl::Log(std::format("GL debug - {}: '{}'\n", severityStr, message));
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
    Ogl::Invoke<Ogl::CharacterEvent>({ .Codepoint = codepoint });
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

//layer methods

void Ogl::AddLayer(Layer* layer)
{
    layer->Index = Layers.size();
    layer->BlockIndex = Vbo.AddBlock();
    Layers.push_back(layer);
}

void Ogl::RemoveLayer(Layer* layer)
{
    Vbo.RemoveBlock(layer->BlockIndex);

    for (int i = layer->Index; i < Layers.size(); i++)
    {
        layer->BlockIndex--;
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
std::tuple<int, int> Ogl::GetWindowSize()
{
    int width, height;
    glfwGetFramebufferSize(Window, &width, &height);
    return { width, height };
}

//sets size of the window's content area in screen coordinates, NOT pixels
void Ogl::SetWindowSize(int width, int height)
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
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
    
    //binding ssbo
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_BINDING, Ssbo.Name);

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

        for (Layer* layer : Layers)
        {
            BufferBlock& layerBlock = Vbo.Blocks[layer->BlockIndex];
            layer->RenderingDataUsed = 0;
            layer->Draw();

            //substituting buffer's data by layer's newly generated one
            size_t dataSize = layer->RenderingDataUsed;
            if (dataSize > 0 || layer->Redraw)
            {
                if (dataSize > layerBlock.Size)
                {
                    Log(std::format("Layer no. {} has exceeded it's memory limit, expanding from {} to {} bytes.\n", layer->Index, layerBlock.Size, dataSize * 2));
                    Vbo.ResizeBlock(layer->BlockIndex, layerBlock.Size * 2 + dataSize);
                }

                layerBlock.Used = dataSize;

                if (dataSize > 0)
                    glBufferSubData(GL_ARRAY_BUFFER, layerBlock.Offset, dataSize, layer->RenderingData);

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
            glDrawArrays(GL_TRIANGLES, layerBlock.Offset / VERT_SIZE, layerBlock.Used / VERT_SIZE);
        }

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }
}
