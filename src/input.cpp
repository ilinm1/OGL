#include "ogl.hpp"

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
