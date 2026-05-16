#include "Document.h"    
#include "Render/IRenderer.h"
#include "Core/Entity/PointEntity.hpp"
#include "Core/Entity/LineEntity.hpp"
#include "Core/Entity/CircleEntity.hpp"
#include "Core/Entity/RectangleEntity.hpp"
#include "Core/Entity/ArcEntity.hpp"
#include "Core/Entity/EllipseEntity.hpp"
#include "Core/Entity/PolylineEntity.hpp"
#include "Core/Object/Object.hpp"
#include "Core/Math/Color4.hpp"
#include "Core/Math/Constants.hpp"
#include <vector> 
#include <memory>
#include <utility>
#include <fstream>
#include <filesystem>
#include "DrawContext.hpp"
namespace MiniCAD
{
    Document::Document(IRenderer& render, float width, float height)
        : m_scene()
        , m_cmdStack()
        , m_viewport(render, width, height)
        , m_overlay(m_viewport)
        , m_picking(m_scene, m_viewport)
        , m_snap()
        , m_currentSnap() 
        , m_editor(m_scene, m_cmdStack, m_viewport, m_overlay,m_picking, m_snap, m_currentSnap)
    {
      
    }

    bool Document::OnInput(const InputEvent& e)
    {   
        m_mouseX = e.MouseX; // 保存鼠标位置
        m_mouseY = e.MouseY; 
        return m_editor.OnInput(e); 
    }

    void Document::Resize(float width, float height)
    {
        m_viewport.Resize(width, height);
    }

    void Document::SetPath(const std::string& path)
    {
        m_path = path;

        // 提取文件名
        auto pos = path.find_last_of("/\\");
        if (pos != std::string::npos)
            m_name = path.substr(pos + 1);
        else
            m_name = path;

    }

    void Document::SetName(const std::string& name)
    {
        m_name = name;
    }

    bool Document::Save()
    {

        if (!HasPath())
        { 
            return false;
        }

        return SaveToFile(m_path);
    }

    bool Document::SaveAs(const std::string& path)
    {
        if (SaveToFile(path))
        {
            SetPath(path);
            m_dirty = false;
            return true;
        }
        return false;
    }

    bool Document::SaveToFile(const std::string& path)
    {

        std::ofstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            printf("Failed to open file: %s\n", path.c_str());
            return false;
        }

        // 写入实体数量
        int count = m_scene.EntityCount();
        file.write((const char*)&count, sizeof(count));

        m_scene.ForEachObject([&](const Object& obj)
            {
                if (obj.IsKindOf<LineEntity>())
                {
                    // 写类型标记
                    uint8_t type = 1; // 1 = LineEntity
                    file.write((const char*)&type, sizeof(type));

                    const auto& line = static_cast<const LineEntity&>(obj);

                    // 写 ID
                    Object::ObjectID id = line.GetID();
                    file.write((const char*)&id, sizeof(id));

                    // 写几何数据
                    const Line& geom = line.GetLine();
                    file.write((const char*)&geom.Start, sizeof(Math::Point3));
                    file.write((const char*)&geom.End, sizeof(Math::Point3));

                    // 写属性
                    const EntityAttr& attr = line.GetAttr();
                    file.write((const char*)&attr.Color, sizeof(Math::Color4));
                    file.write((const char*)&attr.LayerId, sizeof(LayerID));
                    file.write((const char*)&attr.LineType, sizeof(LineType));
                    file.write((const char*)&attr.LineWidth, sizeof(float));
                    file.write((const char*)&attr.Visible, sizeof(bool));
                }
                else if (obj.IsKindOf<PointEntity>())
                {
                    uint8_t type = 2; // 2 = PointEntity
                    file.write((const char*)&type, sizeof(type));

                    const auto& point = static_cast<const PointEntity&>(obj);

                    Object::ObjectID id = point.GetID();
                    file.write((const char*)&id, sizeof(id));

                    const Point& geom = point.GetPoint();
                    file.write((const char*)&geom.Position, sizeof(Math::Point3));

                    const EntityAttr& attr = point.GetAttr();
                    file.write((const char*)&attr.Color, sizeof(Math::Color4));
                    file.write((const char*)&attr.LayerId, sizeof(LayerID));
                    file.write((const char*)&attr.LineType, sizeof(LineType));
                    file.write((const char*)&attr.LineWidth, sizeof(float));
                    file.write((const char*)&attr.Visible, sizeof(bool));
				}// 其他实体类型...
                else if (obj.IsKindOf<RectangleEntity>())
                {
                    uint8_t type = 3;
                    file.write((const char*)&type, sizeof(type));

                    const auto& rect = static_cast<const RectangleEntity&>(obj);

                    Object::ObjectID id = rect.GetID();
                    file.write((const char*)&id, sizeof(id));

                    const Rectangle& geom = rect.GetRectangle();
                    file.write((const char*)&geom.P1, sizeof(Math::Point3));
                    file.write((const char*)&geom.P2, sizeof(Math::Point3));
                    file.write((const char*)&geom.P3, sizeof(Math::Point3));
                    file.write((const char*)&geom.P4, sizeof(Math::Point3));

                    const EntityAttr& attr = rect.GetAttr();
                    file.write((const char*)&attr.Color, sizeof(Math::Color4));
                    file.write((const char*)&attr.LayerId, sizeof(LayerID));
                    file.write((const char*)&attr.LineType, sizeof(LineType));
                    file.write((const char*)&attr.LineWidth, sizeof(float));
                    file.write((const char*)&attr.Visible, sizeof(bool));
                }
                else if (obj.IsKindOf<CircleEntity>())
                {
                    uint8_t type = 4;
                    file.write((const char*)&type, sizeof(type));

                    const auto& circle = static_cast<const CircleEntity&>(obj);

                    Object::ObjectID id = circle.GetID();
                    file.write((const char*)&id, sizeof(id));

                    const Circle& geom = circle.GetCircle();
                    file.write((const char*)&geom.Center, sizeof(Math::Point3));
                    file.write((const char*)&geom.Radius, sizeof(double));  // double，与构造函数一致

                    const EntityAttr& attr = circle.GetAttr();
                    file.write((const char*)&attr.Color, sizeof(Math::Color4));
                    file.write((const char*)&attr.LayerId, sizeof(LayerID));
                    file.write((const char*)&attr.LineType, sizeof(LineType));
                    file.write((const char*)&attr.LineWidth, sizeof(float));
                    file.write((const char*)&attr.Visible, sizeof(bool));
                }
                else if (obj.IsKindOf<ArcEntity>())
                {
                    uint8_t type = 5;
                    file.write((const char*)&type, sizeof(type));

                    const auto& arc = static_cast<const ArcEntity&>(obj);

                    Object::ObjectID id = arc.GetID();
                    file.write((const char*)&id, sizeof(id));

                    const Arc& geom = arc.GetArc();
                    file.write((const char*)&geom.Center, sizeof(Math::Point3));
                    file.write((const char*)&geom.Radius, sizeof(double));
                    file.write((const char*)&geom.StartAngle, sizeof(double));
                    file.write((const char*)&geom.EndAngle, sizeof(double));

                    const EntityAttr& attr = arc.GetAttr();
                    file.write((const char*)&attr.Color, sizeof(Math::Color4));
                    file.write((const char*)&attr.LayerId, sizeof(LayerID));
                    file.write((const char*)&attr.LineType, sizeof(LineType));
                    file.write((const char*)&attr.LineWidth, sizeof(float));
                    file.write((const char*)&attr.Visible, sizeof(bool));
                }
                else if (obj.IsKindOf<EllipseEntity>())
                {
                    uint8_t type = 6;
                    file.write((const char*)&type, sizeof(type));

                    const auto& ellipse = static_cast<const EllipseEntity&>(obj);

                    Object::ObjectID id = ellipse.GetID();
                    file.write((const char*)&id, sizeof(id));

                    const Ellipse& geom = ellipse.GetEllipse();
                    file.write((const char*)&geom.Center, sizeof(Math::Point3));
                    file.write((const char*)&geom.RadiusX, sizeof(double));
                    file.write((const char*)&geom.RadiusY, sizeof(double));
                    file.write((const char*)&geom.Rotation, sizeof(double));

                    const EntityAttr& attr = ellipse.GetAttr();
                    file.write((const char*)&attr.Color, sizeof(Math::Color4));
                    file.write((const char*)&attr.LayerId, sizeof(LayerID));
                    file.write((const char*)&attr.LineType, sizeof(LineType));
                    file.write((const char*)&attr.LineWidth, sizeof(float));
                    file.write((const char*)&attr.Visible, sizeof(bool));
                }
                else if (obj.IsKindOf<PolylineEntity>())
                {
                    uint8_t type = 7;
                    file.write((const char*)&type, sizeof(type));

                    const auto& polyline = static_cast<const PolylineEntity&>(obj);

                    Object::ObjectID id = polyline.GetID();
                    file.write((const char*)&id, sizeof(id));

                    const Polyline& geom = polyline.GetPolyline();

                    uint32_t pointCount = static_cast<uint32_t>(geom.Points.size());
                    file.write((const char*)&pointCount, sizeof(uint32_t));
                    file.write((const char*)geom.Points.data(), sizeof(Math::Point3) * pointCount);

                    uint32_t bulgeCount = static_cast<uint32_t>(geom.Bulges.size());
                    file.write((const char*)&bulgeCount, sizeof(uint32_t));
                    if (bulgeCount > 0)
                        file.write((const char*)geom.Bulges.data(), sizeof(double) * bulgeCount);

                    const EntityAttr& attr = polyline.GetAttr();
                    file.write((const char*)&attr.Color, sizeof(Math::Color4));
                    file.write((const char*)&attr.LayerId, sizeof(LayerID));
                    file.write((const char*)&attr.LineType, sizeof(LineType));
                    file.write((const char*)&attr.LineWidth, sizeof(float));
                    file.write((const char*)&attr.Visible, sizeof(bool));
                }
            });

        printf("Saved to: %s (%d entities)\n", path.c_str(), count);
        m_dirty = false;
        return true;
    }

    void Document::Render()
    { 
        m_overlayVertices.clear();

        UpdateSceneVerties();

        // 判断是否需要绘制约束辅助线 

        if (m_editor.GetGripEditor().IsDragging())
        {
            if (m_editor.IsOrthoEnabled()) // 1.正交 显示约束线
            {
                const Line& anchorLine = m_editor.GetAnchorLine();

                m_overlay.AddLine(anchorLine.Start, anchorLine.End, { 0.1, 0.7, 0.1,0.6 });
            }
            else                          //  2.拖动 显示原来位置
            {
               /* for (const auto& entry : m_editor.GetGripEditor().GetDragEntries())
                {
                    m_overlay.AddLine(entry.BaseLine.Start, entry.BaseLine.End, { 0.6, 0.6, 0.6,0.6 });
                }*/
            }
        }

           
        m_overlay.ToVertices(m_overlayVertices);      // 每帧分配 

        auto vs = BuildViewState();
        m_viewport.Render(vs);
         
    } 
      
    void Document::UpdateSceneVerties()
    { 
        if (!m_scene.IsDirty() && !m_picking.IsDirty())
            return;
         
        m_sceneVertices.clear();
        m_overlay.Clear();  

        const auto& hoverIds     = m_picking.GetHovered();
        const auto& selectionIds = m_picking.GetSelection();

        // 优化：当没有悬停和选择时，清除预览数据并重建夹点，避免残留和状态错误
        if (hoverIds.empty() || selectionIds.empty())
        {
            m_overlay.Clear();
            m_editor.GetGripEditor().RebuildGrips(); // 确保夹点状态正确
        }

        DrawContext ctx(m_sceneVertices, m_overlay);

        m_scene.ForEachObject([&](const Object& obj)
            {
                if (obj.IsKindOf<Entity>())
                {
                    const auto& entity = static_cast<const Entity&>(obj);

                    auto isSelected = selectionIds.contains(obj.GetID());
                    auto isHovered = hoverIds.contains(obj.GetID());
                    entity.Draw(ctx, isSelected, isHovered);
                }

            });

        m_scene.ClearDirty();
        m_picking.ClearDirty();
    }

    ViewState Document::BuildViewState()
    {
        ViewState vs; 

        // 场景点
        vs.Scene            = m_sceneVertices;
        vs.Overlay          = m_overlayVertices; 

        // 选择范围框
        vs.Selection.Active = m_picking.IsBoxSelecting();
        vs.Selection.Start  = m_picking.GetBoxStart();
        vs.Selection.End    = m_picking.GetBoxEnd();  
        
        // 光标位置
        vs.MouseX    = static_cast<float>(m_mouseX);
        vs.MouseY    = static_cast<float>(m_mouseY);  

        // 辅助网格
        vs.ShowGrid  = true;  

        // 最近点 
        vs.Snap.SnapType = static_cast<SnapDraw::Type>(m_currentSnap.SnapType);
        vs.Snap.Pos      = m_viewport.GetCamera().WorldToScreen(m_currentSnap.WorldPos); 
		if (!m_editor.IsActiveTool())
        {
            m_currentSnap = {};// 重置最近点
        }
        // 光标中间方框
        vs.ShowCurrorBox = !m_editor.IsActiveTool();

        // 夹点
        vs.ShowGizmo = true;
        if (vs.ShowGizmo)
        { 
            auto& hoveredIdxs = m_editor.GetGripEditor().HoveredGrips();
            auto& grips = m_editor.GetGripEditor().GetGrips();

            for (int i = 0; i < (int)grips.size(); ++i)
            {
                const auto& g = grips[i];
                auto        s = m_viewport.GetCamera().WorldToScreen(g.WorldPos);
                auto        type = static_cast<GripDraw::Type>(g.GripType);

                // 在列表里找，而不是判断单个 index
                bool hovered = std::find(hoveredIdxs.begin(), hoveredIdxs.end(), i) != hoveredIdxs.end();

                vs.Grips.push_back({ s, type, hovered });
            }
        } 

        return vs;
    }
 
}