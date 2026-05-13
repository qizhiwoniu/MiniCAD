#pragma once
#include "SnapResult.h"
#include "Scene/Scene.h"
#include "Editor/Viewport/Camera.h"
#include "Core/Object/Object.hpp"
#include <unordered_set>
#include <DirectXMath.h>

namespace MiniCAD
{
    class SnapEngine
    {
    public:
        // ─── 开关 ───────────────────────────────
        bool  EnableEndpoint = true;
        bool  EnableMidpoint = true;
        bool  EnableCenter = true;          // 圆心（CircleEntity 接入后生效）
        bool  EnableIntersection = true;    // 交点（Line x Line）
        bool  EnablePerpendicular = false;   // 垂足
        bool  EnableTangent = false;        // 切点（CircleEntity 接入后生效）
        bool  EnableNearest  = true;
        bool  EnableGrid     = false;
        float GridSize       = 1.0f;        // 世界单位
        float SnapRadiusPx   = 12.f;        // 屏幕像素捕捉半径

        // ─── 主接口 ─────────────────────────────
        // exclude: 需要跳过的对象（夹点拖拽时传入当前选中集合，避免捕捉自身）
        SnapResult Query(const DirectX::XMFLOAT2&                        screenPt,
                         const Scene&                                    scene,
                         const Camera&                                   cam,
                         const std::unordered_set<Object::ObjectID>&     exclude = {}) const;

    private:
        SnapResult TryEndpoint(const DirectX::XMFLOAT2& sp, const Scene&, const Camera&,
                               const std::unordered_set<Object::ObjectID>& exclude) const;
        SnapResult TryMidpoint(const DirectX::XMFLOAT2& sp, const Scene&, const Camera&,
                               const std::unordered_set<Object::ObjectID>& exclude) const;
        SnapResult TryIntersection(const DirectX::XMFLOAT2& sp, const Scene&, const Camera&,
                                   const std::unordered_set<Object::ObjectID>& exclude) const;
        SnapResult TryPerpendicular(const DirectX::XMFLOAT2& sp, const Scene&, const Camera&,
                                    const std::unordered_set<Object::ObjectID>& exclude) const;
        SnapResult TryNearest (const DirectX::XMFLOAT2& sp, const Scene&, const Camera&,
                               const std::unordered_set<Object::ObjectID>& exclude) const;
        SnapResult TryGrid    (const DirectX::XMFLOAT2& sp, const Camera&) const;
    };
}
