#pragma once

#include <ogl.hpp>

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

const static char* VertexShaderSource =
    "#version 430 core\n"
    "layout (location = 0) in vec2 Coords;\n"
    "layout (location = 1) in vec2 TextureCoordsIn;\n"
    "layout (location = 2) in uint TextureIndexIn;\n"
    "layout (location = 3) in uint ModulateColorIn;\n"
    "uniform mat3 NDCMatrix;\n"
    "out vec2 TextureCoords;\n"
    "flat out uint TextureIndex;\n"
    "out vec4 ModulateColor;\n"
    "void main()\n"
    "{\n"
    "   TextureCoords = TextureCoordsIn;\n"
    "   TextureIndex = TextureIndexIn;\n"
    "   float modulateR = (ModulateColorIn & 0xFF) / 255.0f;\n"
    "   float modulateG = (ModulateColorIn >> 8 & 0xFF) / 255.0f;\n"
    "   float modulateB = (ModulateColorIn >> 16 & 0xFF) / 255.0f;\n"
    "   float modulateA = (ModulateColorIn >> 24 & 0xFF) / 255.0f;\n"
    "   ModulateColor = vec4(modulateR, modulateG, modulateB, modulateA);\n"
    "   vec3 ndc = NDCMatrix * vec3(Coords.xy, 1.0f);\n"
    "   gl_Position = vec4(ndc.xy, 0.0f, 1.0f);\n"
    "}\n";

const static char* FragmentShaderSource =
    "#version 430 core\n"
    "in vec2 TextureCoords;\n"
    "flat in uint TextureIndex;\n"
    "in vec4 ModulateColor;\n"
    "uniform float DrawingDepth;\n"
    "uniform sampler2D AtlasTexture;\n"
    "layout (binding = " STRINGIFY(SSBO_BINDING) ", std430) buffer TextureDimensionsBuffer\n" //format: x - x, y - y, z - width, w - height
    "{\n"
    "    uvec4 TextureDimensions[];\n"
    "};\n"
    "out vec4 FragColor;\n"
    "out float gl_FragDepth;\n"
    "void main()\n"
    "{\n"
    "   gl_FragDepth = DrawingDepth;\n"
    "   vec2 atlasSize = vec2(textureSize(AtlasTexture, 0));\n"
    "   vec4 texData = vec4(TextureDimensions[TextureIndex]);\n"
    "   texData.x /= atlasSize.x; texData.y /= atlasSize.y; texData.z /= atlasSize.x; texData.w /= atlasSize.y;\n"
    "   vec2 atlasCoords = texData.xy + vec2(mod(TextureCoords.x, 1.0f) * texData.z, mod(TextureCoords.y, 1.0f) * texData.w);\n"
    "   vec4 color = texture(AtlasTexture, atlasCoords)\n;"
    "   float isValidTexture = min(1, TextureIndex)\n;"
    "   color.xyz *= isValidTexture;\n" //color.xyz = isValidTexture ? color.xyz : 0.0f
    "   color.a = max(color.a, 1.0f - isValidTexture);\n" //color.a = isValidTexture ? color.a : 1.0f
    "   FragColor = ModulateColor * color.w + color * (1.0f - ModulateColor.w);\n"
    "}\n";
