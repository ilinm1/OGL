#pragma once

#include "ogl.hpp"

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

const static char* VertexShaderSource =
    "#version 410 core\n"
    "layout (location = 0) in vec2 Coords;\n"
    "layout (location = 1) in vec2 TexCoordsIn;\n"
    "layout (location = 2) in uint TexIndexIn;\n"
    "uniform mat3 NDCMatrix;\n"
    "out vec2 TexCoords;\n"
    "flat out uint TexIndex;\n"
    "void main()\n"
    "{\n"
    "   TexCoords = TexCoordsIn;\n"
    "   TexIndex = TexIndexIn;\n"
    "   vec3 ndc = NDCMatrix * vec3(Coords.xy, 1.0f);\n"
    "   gl_Position = vec4(ndc.xy, 0.0f, 1.0f);\n"
    "}\n";

const static char* FragmentShaderSource =
    "#version 410 core\n"
    "in vec2 TexCoords;\n"
    "flat in uint TexIndex;\n"
    "uniform float DrawingDepth;\n"
    "uniform vec4 TextureDataArray[" STRINGIFY(TEXTURE_MAX) "];\n" //format: x - x, y - y, z - width, w - height
    "uniform sampler2D AtlasTexture;\n"
    "out vec4 FragColor;\n"
    "out float gl_FragDepth;\n"
    "void main()\n"
    "{\n"
    "   gl_FragDepth = DrawingDepth;\n"
    "   vec4 texData = TextureDataArray[TexIndex];\n"
    "   vec2 atlasCoords = texData.xy + vec2(mod(TexCoords.x, 1.0f) * texData.z, mod(TexCoords.y, 1.0f) * texData.w);\n"
    "   FragColor = texture(AtlasTexture, atlasCoords);\n"
    "}\n";
