#include "SnapResult.h"
#include "SnapEngine.h"
#include "Core/Entity/LineEntity.hpp"
#include "Core/Entity/PointEntity.hpp"
#include "Core/Entity/CircleEntity.hpp"
#include "Core/Entity/RectangleEntity.hpp"
#include "Core/Object/Object.hpp"
#include "Core/Math/Point3.hpp"
#include "Core/Math/Constants.hpp"
#include "Scene/Scene.h"
#include "Editor/Viewport/Camera.h"
#include "Core/Math/MathUtils.hpp"
#include <cmath>
#include <algorithm>
#include <limits>
#include <unordered_set>

namespace MiniCAD{

    // ─── 主入口，返回第一个有效结果─────────────────────────────────────────────────────────────── 
    SnapResult SnapEngine::Query(const Math::Point2& sp, const Scene& scene, const Camera& cam,
        const std::unordered_set<Object::ObjectID>& exclude) const
    {
        if (EnableEndpoint) { auto r = TryEndpoint(sp, scene, cam, exclude); if (r.IsValid()) return r; }
        if (EnableMidpoint) { auto r = TryMidpoint(sp, scene, cam, exclude); if (r.IsValid()) return r; }
        if (EnableQuadrant) { auto r = TryQuadrant(sp, scene, cam, exclude); if (r.IsValid()) return r; } // 象限点捕捉放在最近点前面，优先捕捉圆的特征点
        if (EnableNearest) { auto r = TryNearest(sp, scene, cam, exclude); if (r.IsValid()) return r; }
        if (EnableGrid)     return TryGrid(sp, cam);
        return {};
    }

    // ─── Endpoint ─────────────────────────────────────────────────────────────
    SnapResult SnapEngine::TryEndpoint(const Math::Point2& sp, const Scene& scene, const Camera& cam,
        const std::unordered_set<Object::ObjectID>& exclude) const
    {
        SnapResult best;
        double bestDist = std::numeric_limits<double>::max();

        scene.ForEachObject([&](const Object& obj)
            {
                if (exclude.contains(obj.GetID()))
                    return;

                if (obj.IsKindOf<PointEntity>())
                {
                    auto* point = static_cast<const PointEntity*>(&obj);
                    if (!point) return;

                    auto& p = point->GetPoint();
                    double d = Math::Distance(sp, cam.WorldToScreen(p.Position));
                    if (d < SnapRadiusPx && d < bestDist)
                    {
                        bestDist = d;
                        best = { SnapResult::Type::Endpoint, p.Position, obj.GetID() };
                    }
                }

                if (obj.IsKindOf<LineEntity>())
                {
                    auto* line = static_cast<const LineEntity*>(&obj);
                    if (!line) return;

                    for (const Math::Point3& wp : { line->GetLine().Start, line->GetLine().End })
                    {
                        double d = Math::Distance(sp, cam.WorldToScreen(wp));
                        if (d < SnapRadiusPx && d < bestDist)
                        {
                            bestDist = d;
                            best = { SnapResult::Type::Endpoint, wp, obj.GetID() };
                        }
                    }
                }

                if (obj.IsKindOf<RectangleEntity>())
                {
                    auto* rectangleEntity = static_cast<const RectangleEntity*>(&obj);
                    if (!rectangleEntity) return;

                    const auto& rect = rectangleEntity->GetRectangle();

                    for (const auto& p : { rect.P1, rect.P2, rect.P3, rect.P4 })
                    {
                        double d = Math::Distance(sp, cam.WorldToScreen(p));
                        if (d < SnapRadiusPx && d < bestDist)
                        {
                            bestDist = d;
                            best = { SnapResult::Type::Endpoint, p, obj.GetID() };
                        }
                    }
                }

                // ── 圆心捕捉 ──────────────────────────────────────────
                if (obj.IsKindOf<CircleEntity>())
                {
                    auto* circle = static_cast<const CircleEntity*>(&obj);
                    if (!circle) return;

                    const Math::Point3& center = circle->GetCircle().Center;
                    double d = Math::Distance(sp, cam.WorldToScreen(center));
                    if (d < SnapRadiusPx && d < bestDist)
                    {
                        bestDist = d;
                        best = { SnapResult::Type::Endpoint, center, obj.GetID() };
                    }
                }
            });

        return best;
    }

    // ─── Midpoint ─────────────────────────────────────────────────────────────
    SnapResult SnapEngine::TryMidpoint(const Math::Point2& sp, const Scene& scene, const Camera& cam,
        const std::unordered_set<Object::ObjectID>& exclude) const
    {
        SnapResult best;
        double bestDist = std::numeric_limits<double>::max();

        scene.ForEachObject([&](const Object& obj)
            {
                if (exclude.contains(obj.GetID())) return;

                if (obj.IsKindOf<LineEntity>())
                {
                    auto* line = static_cast<const LineEntity*>(&obj);
                    if (!line) return;

                    auto& L = line->GetLine();
                    Math::Point3 mid = Math::Midpoint(L.Start, L.End);

                    double d = Math::Distance(sp, cam.WorldToScreen(mid));
                    if (d < SnapRadiusPx && d < bestDist)
                    {
                        bestDist = d;
                        best = { SnapResult::Type::Midpoint, mid, obj.GetID() };
                    }
                }

                if (obj.IsKindOf<RectangleEntity>())
                {
                    auto* rectangleEntity = static_cast<const RectangleEntity*>(&obj);
                    if (!rectangleEntity) return;

                    const auto& rect = rectangleEntity->GetRectangle();

                    auto mid12 = Math::Midpoint(rect.P1, rect.P2);
                    auto mid23 = Math::Midpoint(rect.P2, rect.P3);
                    auto mid34 = Math::Midpoint(rect.P3, rect.P4);
                    auto mid41 = Math::Midpoint(rect.P4, rect.P1);

                    for (const auto& p : { mid12, mid23, mid34, mid41 })
                    {
                        double d = Math::Distance(sp, cam.WorldToScreen(p));
                        if (d < SnapRadiusPx && d < bestDist)
                        {
                            bestDist = d;
                            best = { SnapResult::Type::Midpoint, p, obj.GetID() };
                        }
                    }
                }

            });

        return best;
    }

    // ─── Nearest ──────────────────────────────────────────────────────────────
    SnapResult SnapEngine::TryNearest(const Math::Point2& sp, const Scene& scene, const Camera& cam,
        const std::unordered_set<Object::ObjectID>& exclude) const
    {
        SnapResult best;
        double bestDist = std::numeric_limits<double>::max();

        Math::Point3 worldMouse = cam.ScreenToWorld(sp.x, sp.y);

        scene.ForEachObject([&](const Object& obj)
            {
                if (exclude.contains(obj.GetID())) return;

                if (obj.IsKindOf<LineEntity>())
                {
                    auto* line = static_cast<const LineEntity*>(&obj);
                    if (!line) return;

                    auto& L = line->GetLine();
                    Math::Point3 closest = Math::ClosestPointOnSegment(worldMouse, L.Start, L.End);

                    double d = Math::Distance(sp, cam.WorldToScreen(closest));
                    if (d < SnapRadiusPx && d < bestDist)
                    {
                        bestDist = d;
                        best = { SnapResult::Type::Nearest, closest, obj.GetID() };
                    }
                }

                if (obj.IsKindOf<RectangleEntity>())
                {
                    auto* rectangleEntity = static_cast<const RectangleEntity*>(&obj);
                    if (!rectangleEntity) return;
                    const auto& rect = rectangleEntity->GetRectangle();

                    // 计算矩形四条边的最近点
                    Math::Point3 closest12 = Math::ClosestPointOnSegment(worldMouse, rect.P1, rect.P2);
                    Math::Point3 closest23 = Math::ClosestPointOnSegment(worldMouse, rect.P2, rect.P3);
                    Math::Point3 closest34 = Math::ClosestPointOnSegment(worldMouse, rect.P3, rect.P4);
                    Math::Point3 closest41 = Math::ClosestPointOnSegment(worldMouse, rect.P4, rect.P1);
                    for (const auto& p : { closest12, closest23, closest34, closest41 })
                    {
                        double d = Math::Distance(sp, cam.WorldToScreen(p));
                        if (d < SnapRadiusPx && d < bestDist)
                        {
                            bestDist = d;
                            best = { SnapResult::Type::Nearest, p, obj.GetID() };
                        }
                    }

                }

                // 圆上最近点：鼠标世界坐标 → 圆心方向单位向量 → 投影到圆上
                if (obj.IsKindOf<CircleEntity>())
                {
                    auto* circle = static_cast<const CircleEntity*>(&obj);
                    if (!circle) return;

                    const Math::Point3& c = circle->GetCircle().Center;
                    const double        r = circle->GetCircle().Radius;

                    // 鼠标世界坐标 → 圆心方向单位向量 → 投影到圆上
                    double dx = worldMouse.x - c.x;
                    double dy = worldMouse.y - c.y;
                    double len = std::sqrt(dx * dx + dy * dy);
                    if (len < 1e-10) return;   // 鼠标恰好在圆心，跳过

                    Math::Point3 onCircle =
                    {
                        c.x + r * (dx / len),
                        c.y + r * (dy / len),
                        c.z
                    };

                    double d = Math::Distance(sp, cam.WorldToScreen(onCircle));
                    if (d < SnapRadiusPx && d < bestDist)
                    {
                        bestDist = d;
                        best = { SnapResult::Type::Nearest, onCircle, obj.GetID() };
                    }
                }
            });

        return best;
    }
    SnapResult SnapEngine::TryQuadrant(const Math::Point2& sp, const Scene& scene, const Camera& cam, const std::unordered_set<Object::ObjectID>& exclude) const

    {
        SnapResult best;
        double bestDist = std::numeric_limits<double>::max();

        scene.ForEachObject([&](const Object& obj)
            {
                if (exclude.contains(obj.GetID())) return;
                if (!obj.IsKindOf<CircleEntity>()) return;

                auto* circle = static_cast<const CircleEntity*>(&obj);
                if (!circle) return;

                const Math::Point3& c = circle->GetCircle().Center;
                const double        r = circle->GetCircle().Radius;

                // 四个象限点：0° 90° 180° 270°
                const Math::Point3 quadrants[4] =
                {
                    { c.x + r, c.y,     c.z },   // 右
                    { c.x,     c.y + r, c.z },   // 上
                    { c.x - r, c.y,     c.z },   // 左
                    { c.x,     c.y - r, c.z }    // 下
                };

                for (const Math::Point3& qp : quadrants)
                {
                    double d = Math::Distance(sp, cam.WorldToScreen(qp));
                    if (d < SnapRadiusPx && d < bestDist)
                    {
                        bestDist = d;
                        best = { SnapResult::Type::Quadrant, qp, obj.GetID() };
                    }
                }
            });

        if (best.IsValid())
        {

            printf(" SnapResult::Type::Quadrant\n");
        }


        return best;
    }

    // ─── Intersection ─────────────────────────────────────────────────────────
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


    // ─── Perpendicular ────────────────────────────────────────────────────────

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

					 
                    // 不知道什么原因 垂足能一直进来

                    bestDist = distPx;
                    best = { SnapResult::Type::Perpendicular, foot, obj.GetID() };
                }
            });

        return best;
    }

    // ─── Grid ─────────────────────────────────────────────────────────────────
    SnapResult SnapEngine::TryGrid(const Math::Point2& sp, const Camera& cam) const
    {
        Math::Point3 w = cam.ScreenToWorld(sp.x, sp.y);
        return
        {
            SnapResult::Type::Grid,
            {
                std::round(w.x / GridSize) * GridSize,
                std::round(w.y / GridSize) * GridSize,
                0.0
            },
            Object::InvalidID
        };
    }

}  
