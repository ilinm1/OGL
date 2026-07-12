#include "ogl/ogl.hpp"

struct ImageViewerLayer : Ogl::Layer
{
    Ogl::Texture Texture;

    ImageViewerLayer()
    {
        Subscribe<Ogl::KeyPressEvent>(&OnKeyPress);
    }

    static bool OnKeyPress(Ogl::KeyPressEvent& ev, void* data)
    {
        ImageViewerLayer* layer = reinterpret_cast<ImageViewerLayer*>(data);

        if (ev.Action == GLFW_PRESS && ev.Key == GLFW_KEY_ENTER)
        {
            std::filesystem::path path;
            if (Ogl::OpenFilePicker("Load image", false, path))
            {
                layer->Texture = Ogl::ResolveTexture(path);
                Ogl::TextureDimensions textureDimensions = Ogl::TextureDimensionsVector[layer->Texture.Index];
                Ogl::SetWindowSize(textureDimensions.Width, textureDimensions.Height);
            }

            return true;
        }

        return false;
    }

    void Draw() override
    {
        if (Texture.Index == 0)
            return;

        DrawRect(Ogl::Vec2(-1), Ogl::Vec2(1), COLOR_TRANSPARENT, Texture, false);
    }
};

int main()
{
    Ogl::Initialize(500, 500, "ImageViewer", false);

    ImageViewerLayer imageViewerLayer = {};
    Ogl::AddLayer(&imageViewerLayer);

    Ogl::UpdateLoop();
    return 0;
}
