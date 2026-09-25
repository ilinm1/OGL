#pragma once

#include <vector>
#include "glad/glad.h"

//'BufferBlock' and 'Buffer' classes

namespace Tc::Graphics
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
}
