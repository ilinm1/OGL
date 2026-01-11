#include <ogl.hpp>

//input methods

Vec2 Ogl::GetCursorPos()
{
    double cx, cy;
    glfwGetCursorPos(Window, &cx, &cy);
    return Vec2(static_cast<float>(cx), static_cast<float>(cy));
}

//returns true if key's last reported state is 'GLFW_PRESS'
//key is one of GLFW's key IDs
bool Ogl::IsKeyPressed(int key)
{
    return glfwGetKey(Window, key) == GLFW_PRESS;
}

bool Ogl::IsMouseButtonPressed(int button)
{
    return glfwGetMouseButton(Window, button) == GLFW_PRESS;
}

std::string Ogl::GetClipboardContents()
{
    const char* ptr = glfwGetClipboardString(NULL);
    if (ptr == NULL)
        return "";
    return std::string(ptr);
}

//opens a file picker window with the title 'title'
//if the purpouse of the picker is to select the file for writing then 'write' may be set
//in which case various additional settings will be applied (ovewrite warnings, ignoring readonly files, etc.)
//if user has chosen a valid file then method will return true and set 'path' reference to the path of the selected file
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
bool Ogl::OpenFilePicker(std::string title, bool write, std::filesystem::path& path)
{
    std::filesystem::path result;
    std::string pathStr = std::string(256, 0);

    //https://learn.microsoft.com/en-us/windows/win32/api/commdlg/ns-commdlg-openfilenamea
    OPENFILENAMEA schizoStruct = {
        sizeof(OPENFILENAMEA),
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        1,
        pathStr.data(),
        pathStr.capacity(),
        NULL,
        0,
        NULL,
        title.c_str(),
        OFN_FILEMUSTEXIST | (write ? OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN : 0),
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL
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
std::filesystem::path OpenFilePicker()
{
    throw std::runtime_error("Not implemented for your OS.");
}
#endif
