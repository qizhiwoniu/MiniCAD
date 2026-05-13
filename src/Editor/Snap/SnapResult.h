#pragma once
#include "Core/Object/Object.hpp"
#include <DirectXMath.h>
#include <cstdint>

namespace MiniCAD
{
    struct SnapResult
    {
        enum class Type : uint8_t { None, Endpoint, Midpoint, Intersection, Perpendicular, Nearest, Grid };
        Type              SnapType = Type::None;
        DirectX::XMFLOAT3 WorldPos = {};
        Object::ObjectID  SourceID = Object::InvalidID;

        bool IsValid() const { return SnapType != Type::None; }
    };
}
