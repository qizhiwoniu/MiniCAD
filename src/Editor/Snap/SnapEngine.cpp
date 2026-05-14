#include "SnapResult.h"
#include "SnapEngine.h"
#include "Core/Entity/LineEntity.hpp"
#include "Core/Entity/PointEntity.hpp"
#include "Core/Object/Object.hpp"
#include "Scene/Scene.h"
#include "Editor/Viewport/Camera.h"
#include <cmath>
#include <algorithm>
#include <unordered_set>

using namespace DirectX;

namespace MiniCAD
{
    // ─── 内部工具 ─────────────────────────────────────────────────────────────
    static float Dist2D(const XMFLOAT2& a, const XMFLOAT2& b)
    {
        return std::hypot(a.x - b.x, a.y - b.y);
    }

    static XMFLOAT3 ClosestPointOnSegment(const XMFLOAT3& p, const XMFLOAT3& a, const XMFLOAT3& b)
    {
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        float lenSq = dx * dx + dy * dy;

        if (lenSq < 1e-8f) return a;

        float t = std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / lenSq, 0.f, 1.f);
        return { a.x + t * dx, a.y + t * dy, 0.f };
    }

    // ─── 主入口 ───────────────────────────────────────────────────────────────
    SnapResult SnapEngine::Query(const XMFLOAT2& sp, const Scene& scene, const Camera& cam,
        const std::unordered_set<Object::ObjectID>& exclude) const
    {
        // 优先级：端点 > 中点 > 交点 > 垂足 > 最近点 > 网格
        if (EnableEndpoint) { auto r = TryEndpoint(sp, scene, cam, exclude); if (r.IsValid()) return r; }
        if (EnableMidpoint) { auto r = TryMidpoint(sp, scene, cam, exclude); if (r.IsValid()) return r; }
        if (EnableIntersection) { auto r = TryIntersection(sp, scene, cam, exclude); if (r.IsValid()) return r; }
        if (EnablePerpendicular) { auto r = TryPerpendicular(sp, scene, cam, exclude); if (r.IsValid()) return r; }
        if (EnableNearest)  { auto r = TryNearest (sp, scene, cam, exclude); if (r.IsValid()) return r; }
        if (EnableGrid)     return TryGrid(sp, cam);
        return {};
    }

    // ─── Endpoint ─────────────────────────────────────────────────────────────
    SnapResult SnapEngine::TryEndpoint(const XMFLOAT2& sp, const Scene& scene, const Camera& cam,
        const std::unordered_set<Object::ObjectID>& exclude) const
    {
        SnapResult best;
        float bestDist = FLT_MAX;

        scene.ForEachObject([&](const Object& obj)
            {
                if (exclude.contains(obj.GetID())) return;  //  跳过选中对象

                if (obj.IsKindOf<LineEntity>())
                {
                    auto* line = static_cast<const LineEntity*>(&obj);
                    if (!line) return;

                    for (const XMFLOAT3& wp : { line->GetLine().Start, line->GetLine().End })
                    {
                        float d = Dist2D(sp, cam.WorldToScreen(wp));
                        if (d < SnapRadiusPx && d < bestDist)
                        {
                            bestDist = d;
                            best = { SnapResult::Type::Endpoint, wp, obj.GetID() };
                        }
                    }
                }

                if (obj.IsKindOf<PointEntity>())
                {
                    auto* point = static_cast<const PointEntity*>(&obj);
                    if (!point) return;

                    auto& p = point->GetPoint();

                    float d = Dist2D(sp, cam.WorldToScreen(p.Position));  // 计算距离
                    if (d < SnapRadiusPx && d < bestDist)                 // 双重判断
                    {
                        bestDist = d;
                        best = { SnapResult::Type::Endpoint, p.Position, obj.GetID() };
                    }  
                } 
            });

        return best;
    }

    // ─── Midpoint ─────────────────────────────────────────────────────────────
    SnapResult SnapEngine::TryMidpoint(const XMFLOAT2& sp, const Scene& scene, const Camera& cam,
        const std::unordered_set<Object::ObjectID>& exclude) const
    {
        SnapResult best;
        float bestDist = FLT_MAX;

        scene.ForEachObject([&](const Object& obj)
            {
                if (exclude.contains(obj.GetID())) return;  // 跳过选中对象

                if (obj.IsKindOf<LineEntity>())
                {
                    auto* line = static_cast<const LineEntity*>(&obj);
                    if (!line) return;

                    auto& L = line->GetLine();
                    XMFLOAT3 mid = { (L.Start.x + L.End.x) * 0.5f, (L.Start.y + L.End.y) * 0.5f, 0.f };

                    float d = Dist2D(sp, cam.WorldToScreen(mid));
                    if (d < SnapRadiusPx && d < bestDist)
                    {
                        bestDist = d;
                        best = { SnapResult::Type::Midpoint, mid, obj.GetID() };
                    }
                }
              
               
            });

        return best;
    }

    // ─── Nearest ──────────────────────────────────────────────────────────────
    SnapResult SnapEngine::TryNearest(const XMFLOAT2& sp, const Scene& scene, const Camera& cam,
        const std::unordered_set<Object::ObjectID>& exclude) const
    {
        SnapResult best;
        float bestDist = FLT_MAX;

        XMFLOAT3 worldMouse = cam.ScreenToWorld(sp.x, sp.y);

        scene.ForEachObject([&](const Object& obj)
            {
                if (exclude.contains(obj.GetID())) return;  // 跳过选中对象

                if (obj.IsKindOf<LineEntity>())
                {
                    auto* line = static_cast<const LineEntity*>(&obj);
                    if (!line) return;

                    auto& L = line->GetLine();
                    XMFLOAT3 closest = ClosestPointOnSegment(worldMouse, L.Start, L.End);

                    float d = Dist2D(sp, cam.WorldToScreen(closest));
                    if (d < SnapRadiusPx && d < bestDist)
                    {
                        bestDist = d;
                        best = { SnapResult::Type::Nearest, closest, obj.GetID() };
                    }

                }   

            });

        return best;
    }
    // ─── Intersection ─────────────────────────────────────────────────────────
    // 两条线段（无限延长）的交点，结果必须落在两段的包围盒范围内才接受
    SnapResult SnapEngine::TryIntersection(const XMFLOAT2& sp, const Scene& scene, const Camera& cam,
        const std::unordered_set<Object::ObjectID>& exclude) const
    {
        // 收集所有线段
        std::vector<const LineEntity*> lines;
        scene.ForEachObject([&](const Object& obj)
            {
                if (exclude.contains(obj.GetID())) return;
                if (obj.IsKindOf<LineEntity>())
                    lines.push_back(static_cast<const LineEntity*>(&obj));
            });

<<<<<<<<< Temporary merge branch 1
=========
    // ─── Intersection ─────────────────────────────────────────────────────────
    // 两条线段（无限延长）的交点，结果必须落在两段的包围盒范围内才接受
    SnapResult SnapEngine::TryIntersection(const XMFLOAT2& sp, const Scene& scene, const Camera& cam,
        const std::unordered_set<Object::ObjectID>& exclude) const
    {
        // 收集所有线段
        std::vector<const LineEntity*> lines;
        scene.ForEachObject([&](const Object& obj)
            {
                if (exclude.contains(obj.GetID())) return;
                if (obj.IsKindOf<LineEntity>())
                    lines.push_back(static_cast<const LineEntity*>(&obj));
            });

        SnapResult best;
        float bestDist = FLT_MAX;

        // 枚举所有线对
        for (size_t i = 0; i < lines.size(); ++i)
            for (size_t j = i + 1; j < lines.size(); ++j)
            {
                const auto& A = lines[i]->GetLine();
                const auto& B = lines[j]->GetLine();

                // 线段 A: P = A.Start + t*(A.End - A.Start)
                // 线段 B: Q = B.Start + u*(B.End - B.Start)
                // 求 t, u 使 P == Q
                float r_x = A.End.x - A.Start.x, r_y = A.End.y - A.Start.y;  // 方向 A
                float s_x = B.End.x - B.Start.x, s_y = B.End.y - B.Start.y;  // 方向 B

                float denom = r_x * s_y - r_y * s_x;   // r × s
                if (std::fabs(denom) < 1e-8f) continue; // 平行或共线

                float qp_x = B.Start.x - A.Start.x;
                float qp_y = B.Start.y - A.Start.y;

                float t = (qp_x * s_y - qp_y * s_x) / denom;  // (q-p) × s / (r × s)
                float u = (qp_x * r_y - qp_y * r_x) / denom;  // (q-p) × r / (r × s)

                // 两个参数都必须在 [0,1]，交点才在线段上
                if (t < 0.f || t > 1.f || u < 0.f || u > 1.f) continue;

                XMFLOAT3 wp = { A.Start.x + t * r_x, A.Start.y + t * r_y, 0.f };
                float d = Dist2D(sp, cam.WorldToScreen(wp));
                if (d < SnapRadiusPx && d < bestDist)
                {
                    bestDist = d;
                    best = { SnapResult::Type::Intersection, wp, lines[i]->GetID() };
                }
            }

        return best;
    }

<<<<<<<<< Temporary merge branch 1

    // ─── Perpendicular ────────────────────────────────────────────────────────
    // 从鼠标位置向每条线段作垂线，捕捉垂足点
=========
    // ─── Perpendicular ────────────────────────────────────────────────────────
>>>>>>>>> Temporary merge branch 2
    SnapResult SnapEngine::TryPerpendicular(const XMFLOAT2& sp, const Scene& scene, const Camera& cam,
        const std::unordered_set<Object::ObjectID>& exclude) const
    {
        SnapResult best;
        float bestDist = FLT_MAX;

        XMFLOAT3 worldMouse = cam.ScreenToWorld(sp.x, sp.y);

        scene.ForEachObject([&](const Object& obj)
            {
                if (exclude.contains(obj.GetID())) return;
                if (!obj.IsKindOf<LineEntity>()) return;

                auto* line = static_cast<const LineEntity*>(&obj);
                auto& L = line->GetLine();

                // 垂足 = 鼠标到线段（无限延长方向）的投影点
                float dx = L.End.x - L.Start.x, dy = L.End.y - L.Start.y;
                float lenSq = dx * dx + dy * dy;
                if (lenSq < 1e-8f) return;
                float len = std::sqrt(lenSq);

                // 鼠标到直线（无限延长）的垂直距离（世界坐标）
                float crossZ = (worldMouse.x - L.Start.x) * dy - (worldMouse.y - L.Start.y) * dx;
                float distWorld = std::fabs(crossZ) / len;

                // 把世界距离换算成屏幕像素距离来与 SnapRadiusPx 比较
                // 用 cam.Scale() 获取缩放比，若无此接口则用近似：取两个世界点的屏幕距离
                XMFLOAT2 refA = cam.WorldToScreen(L.Start);
                XMFLOAT2 refB = cam.WorldToScreen({ L.Start.x + dx / len, L.Start.y + dy / len, 0.f });
                float pixelsPerUnit = Dist2D(refA, refB);   // 1 个世界单位对应多少像素
                float distPx = distWorld * pixelsPerUnit;

                if (distPx > SnapRadiusPx) return;  // 鼠标离线太远，不吸附

                // 计算垂足（投影到无限延长线上，不 clamp，CAD 标准行为）
                float t = ((worldMouse.x - L.Start.x) * dx + (worldMouse.y - L.Start.y) * dy) / lenSq;
                XMFLOAT3 foot = { L.Start.x + t * dx, L.Start.y + t * dy, 0.f };

                // 以鼠标屏幕位置到垂足屏幕位置的距离作为 best 竞争依据
                float d = Dist2D(sp, cam.WorldToScreen(foot));
                if (distPx < bestDist)
                {
<<<<<<<<< Temporary merge branch 1
=========
					 
                    // 不知道什么原因 垂足能一直进来
>>>>>>>>> Temporary merge branch 2
                    bestDist = distPx;
                    best = { SnapResult::Type::Perpendicular, foot, obj.GetID() };
                }
            });

        return best;
    }
<<<<<<<<< Temporary merge branch 1
=========

>>>>>>>>> Temporary merge branch 2
    // ─── Grid ─────────────────────────────────────────────────────────────────
    SnapResult SnapEngine::TryGrid(const XMFLOAT2& sp, const Camera& cam) const
    {
        XMFLOAT3 w = cam.ScreenToWorld(sp.x, sp.y);
        return
        {
            SnapResult::Type::Grid,
            {
                std::round(w.x / GridSize) * GridSize,
                std::round(w.y / GridSize) * GridSize,
                0.f
            },
            Object::InvalidID
        };
    }

}  
