#include "../include/ogl.hpp"

//camera methods

//updates matrix which translates world coordinates to normalized device coordinates, should be called after changing camera parameters
void Ogl::UpdateWorldToNDCMatrix()
{
    float x = CameraPosition.X;
    float y = CameraPosition.Y;
    float w = CameraSize.X * CameraScale;
    float h = CameraSize.Y * CameraScale;
    float c = cosf(CameraRotation);
    float s = sinf(CameraRotation);

    WorldToNDCMatrix =
    {
        2 * c / w, -2 * s / h, -2 * x * c / w + 2 * y * s / h,
        2 * s / w, 2 * c / h, -2 * x * s / w - 2 * y * c / h,
        0, 0, 0
    };

    WorldToNDCMatrix = WorldToNDCMatrix.AsColumnMajor();
}

//updates matrix which translates normalized device coordinates to pixels, should be called after changing window size
void Ogl::UpdateNDCToPixelMatrix(unsigned int width, unsigned int height)
{
    float w = static_cast<float>(width) / 2.0f;
    float h = static_cast<float>(height) / 2.0f;

    NDCToPixelMatrix =
    {
        w, 0, 0,
        0, h, 0,
        0, 0, 0
    };

    NDCToPixelMatrix = NDCToPixelMatrix.AsColumnMajor();
}

//in in-world meters
void Ogl::SetCameraPosition(Vec2 position)
{
    CameraPosition = position;
    UpdateWorldToNDCMatrix();
}

//in in-world meters
void Ogl::SetCameraSize(Vec2 size)
{
    CameraSize = size;
    UpdateWorldToNDCMatrix();
}

//in radians
void Ogl::SetCameraRotation(float rotation)
{
    CameraRotation = rotation;
    UpdateWorldToNDCMatrix();
}

//1 is 100% of normal camera size
void Ogl::SetCameraScale(float zoom)
{
    CameraScale = zoom;
    UpdateWorldToNDCMatrix();
}

//converts point in NDC/world space to pixels
Vec2 Ogl::PointToPixels(Vec2 point, bool inWorld)
{
    if (inWorld)
        point = WorldToNDCMatrix.TransformVector(point);
    return NDCToPixelMatrix.TransformVector(point);
}

//converts point in pixels to NDC/in-world meters
Vec2 Ogl::PointFromPixels(Vec2 point, bool inWorld)
{
    point = NDCToPixelMatrix.Inverse().TransformVector(point);
    if (inWorld)
        point = WorldToNDCMatrix.Inverse().TransformVector(point);
    return point;
}
