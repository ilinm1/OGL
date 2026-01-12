#pragma once

#include <vec2.hpp>

//3x3 matrix
struct Mat3
{
    float Cells[9];

    Vec2 TransformVector(Vec2 v)
    {
        return Vec2 
        { 
            v.Dotp(*reinterpret_cast<Vec2*>(Cells)) + Cells[2],
            v.Dotp(*reinterpret_cast<Vec2*>(Cells + 3)) + Cells[5]
        };
    }

    //elements in glsl matrices are stored contiguosly in columns (as opposed to rows)
    Mat3 AsColumnMajor()
    {
        return Mat3 { Cells[0], Cells[3], Cells[6], Cells[1], Cells[4], Cells[7], Cells[2], Cells[5], Cells[8] };
    }
};

#define IDENTITY_MATRIX Mat3 { 1, 0, 0, 0, 1, 0, 0, 0, 1 }
