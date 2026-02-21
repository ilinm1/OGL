#include <ogl.hpp>

//camera methods

//updates matrix which translates world coordinates to normalized device coordinates, should be called after changing camera parameters
void Ogl::UpdateWorldToNDCMatrix()
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
void Ogl::UpdateNDCToPixelMatrix(unsigned int width, unsigned int height)
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
        1/w, 0, -1,
        0, -1/h, 1,
        0, 0, 0
    };
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

//converts a point in NDC/world coordinates to pixels
Vec2 Ogl::PointToPixels(Vec2 point, bool inWorld)
{
    if (inWorld)
        point = WorldToNDCMatrix.TransformVector(point);
    return NDCToPixelMatrix.TransformVector(point);
}

//converts a point in pixels to NDC/world coordinates
Vec2 Ogl::PointFromPixels(Vec2 point, bool inWorld)
{
    point = PixelToNDCMatrix.TransformVector(point);
    if (inWorld)
        point = NDCToWorldMatrix.TransformVector(point);
    return point;
}

//unlike 'PointToPixels' doesn't account for camera's position, rotation, different coordinate centers, etc. 
//only converting the actual dimensions of the object
Vec2 Ogl::SizeToPixels(Vec2 size, bool inWorld)
{
    auto [width, height] = Ogl::GetWindowSize();
    size = size.Rotated(Ogl::CameraRotation);
    size += Ogl::CameraPosition;
    size = Ogl::PointToPixels(size, inWorld);
    size.X -= width / 2;
    size.Y = height / 2 - size.Y;
    return size.Abs();
}

//unlike 'PointFromPixels' doesn't account for camera's position, rotation, different coordinate centers, etc. 
//only converting the actual dimensions of the object
Vec2 Ogl::SizeFromPixels(Vec2 size, bool inWorld)
{
    size = Ogl::PointFromPixels(size, inWorld);
    size += Vec2(Ogl::CameraSize.X, -Ogl::CameraSize.Y) / 2;
    size -= Ogl::CameraPosition;
    size = size.Rotated(-Ogl::CameraRotation);
    return size.Abs();
}
