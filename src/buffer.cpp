#include <ogl.hpp>

//not doing all of this in constructor since glad must be initialized beforehand
void Ogl::Buffer::Initialize(
    unsigned int name,
    unsigned int copyName,
    unsigned int size,
    unsigned int usage,
    unsigned int binding)
{
    if (name == 0)
        glGenBuffers(1, &Name);

    CopyName = copyName;
    Size = size;
    Usage = usage;
    Binding = binding;

    glBindBuffer(binding, Name);
    glBufferData(binding, size, NULL, Usage);
}

size_t Ogl::Buffer::AddBlock(unsigned int size)
{
    BufferBlock block;
    if (Blocks.empty())
    {
        block = { 0, size };
    }
    else
    {
        BufferBlock lastBlock = Blocks.back();
        block = { lastBlock.Offset + lastBlock.Size, size };
    }

    Blocks.push_back(block);
    ResizeBlock(Blocks.size() - 1, size);
    return Blocks.size() - 1;
}

void Ogl::Buffer::ResizeBlock(size_t index, unsigned int size)
{
    BufferBlock& block = Blocks[index];

    if (block.Size == size)
        return;

    unsigned int used = Blocks.back().Offset + Blocks.back().Size;
    if (used + size - block.Size > Size)
        throw std::runtime_error("Out of video memory.");

    //copying data after the block to it's new end position
    unsigned int copySize = used - block.Offset - block.Size;
    glBindBuffer(GL_COPY_READ_BUFFER, Name);
    glBindBuffer(GL_COPY_WRITE_BUFFER, CopyName);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, block.Offset + block.Size, 0, copySize);
    glCopyBufferSubData(GL_COPY_WRITE_BUFFER, GL_COPY_READ_BUFFER, 0, block.Offset + size, copySize);

    for (int i = index + 1; i < Blocks.size(); i++)
    {
        Blocks[i].Offset += size - block.Size;
    }

    block.Size = size;
}

void Ogl::Buffer::RemoveBlock(size_t index)
{
    ResizeBlock(index, 0);
    Blocks.erase(Blocks.begin() + index);
}
