#include "DocumentManager.h"
#include "Document.h"
#include "Core/Entity/PointEntity.hpp"
#include "Core/Entity/CircleEntity.hpp"
#include "Core/Entity/RectangleEntity.hpp"
#include "Core/Entity/ArcEntity.hpp"
#include "Core/Entity/EllipseEntity.hpp"
#include "Core/Entity/PolylineEntity.hpp"
#include <utility>
#include <memory>
#include <string>
#include <filesystem>
#include <windows.h>
#include <commdlg.h>
#include <fstream>

namespace MiniCAD
{
    Document& DocumentManager::Create(IRenderer& r, float w, float h)
    {
        auto doc = std::make_unique<Document>(r, w, h);

        doc->SetName(GenerateUniqueName());

        m_active = doc.get();
        m_docs.push_back(std::move(doc));

        return *m_active;
    }

    void DocumentManager::Close(Document* doc)
    {
        auto it = std::find_if(m_docs.begin(), m_docs.end(),
            [&](const auto& d) { return d.get() == doc; });

        if (it == m_docs.end())
            return;

        if (m_active == doc)
            m_active = nullptr;

        m_docs.erase(it);

        if (!m_docs.empty() && m_active == nullptr)
            m_active = m_docs.back().get();
    }

    Document* DocumentManager::GetActive() const
    {
        return m_active;
    }

    void DocumentManager::SetActive(Document* doc)
    {
        m_active = doc;
    }

    std::vector<std::unique_ptr<Document>>& DocumentManager::GetAll()
    {
        return m_docs; 
    }

    void DocumentManager::SetRenderer(IRenderer* renderer)
    {
        m_renderer = renderer; 
    }

    void DocumentManager::New()
    {
        if (!m_renderer)
            return;

        Create(*m_renderer, m_defaultWidth, m_defaultHeight);
    }

    void DocumentManager::Open()
    {
        char filePath[MAX_PATH] = {};

        OPENFILENAMEA ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFilter = "DWG Files (*.dwg)\0*.dwg\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = filePath;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = "Open DWG File";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        if (!GetOpenFileNameA(&ofn))
            return;

        if (!m_renderer)
            return;

        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            printf("Failed to open file: %s\n", filePath);
            return;
        }

        Document& doc = Create(*m_renderer, m_defaultWidth, m_defaultHeight);
        doc.SetPath(filePath);
        Scene& scene = doc.GetScene();

        int count = 0;
        file.read((char*)&count, sizeof(count));

        for (int i = 0; i < count; i++)
        {
            uint8_t type = 0;
            file.read((char*)&type, sizeof(type));

            Object::ObjectID id = 0;
            file.read((char*)&id, sizeof(id));

            if (type == 1) // LineEntity
            {
                Math::Point3 start, end; 
                file.read((char*)&start, sizeof(Math::Point3));
                file.read((char*)&end, sizeof(Math::Point3)); 

                EntityAttr attr;
                file.read((char*)&attr.Color, sizeof(Math::Color4));
                file.read((char*)&attr.LayerId, sizeof(LayerID));
                file.read((char*)&attr.LineType, sizeof(LineType));
                file.read((char*)&attr.LineWidth, sizeof(float));
                file.read((char*)&attr.Visible, sizeof(bool));

                auto entity = std::make_unique<LineEntity>(id, start, end);
                entity->SetAttr(attr);
                scene.AddEntity(std::move(entity));
            }
            else if (type == 2) // PointEntity
            {
                Math::Point3 pos;
                file.read((char*)&pos, sizeof(Math::Point3));

                EntityAttr attr;
                file.read((char*)&attr.Color, sizeof(Math::Color4));
                file.read((char*)&attr.LayerId, sizeof(LayerID));
                file.read((char*)&attr.LineType, sizeof(LineType));
                file.read((char*)&attr.LineWidth, sizeof(float));
                file.read((char*)&attr.Visible, sizeof(bool));

                auto entity = std::make_unique<PointEntity>(id, pos);
                entity->SetAttr(attr);
                scene.AddEntity(std::move(entity));
            }
            else if (type == 3) // RectEntity
            {
                Math::Point3 p1, p2, p3, p4;
                file.read((char*)&p1, sizeof(Math::Point3));
                file.read((char*)&p2, sizeof(Math::Point3));
                file.read((char*)&p3, sizeof(Math::Point3));
                file.read((char*)&p4, sizeof(Math::Point3));

                EntityAttr attr;
                file.read((char*)&attr.Color, sizeof(Math::Color4));
                file.read((char*)&attr.LayerId, sizeof(LayerID));
                file.read((char*)&attr.LineType, sizeof(LineType));
                file.read((char*)&attr.LineWidth, sizeof(float));
                file.read((char*)&attr.Visible, sizeof(bool));

                auto entity = std::make_unique<RectangleEntity>(id, p1, p2, p3, p4);
                entity->SetAttr(attr);
                scene.AddEntity(std::move(entity));
            }
            else if (type == 4) // CircleEntity
            {
                Math::Point3 center;
                double radius = 0.0;
                file.read((char*)&center, sizeof(Math::Point3));
                file.read((char*)&radius, sizeof(double));

                EntityAttr attr;
                file.read((char*)&attr.Color, sizeof(Math::Color4));
                file.read((char*)&attr.LayerId, sizeof(LayerID));
                file.read((char*)&attr.LineType, sizeof(LineType));
                file.read((char*)&attr.LineWidth, sizeof(float));
                file.read((char*)&attr.Visible, sizeof(bool));

                auto entity = std::make_unique<CircleEntity>(id, center, radius);
                entity->SetAttr(attr);
                scene.AddEntity(std::move(entity));
            }
            else if (type == 5) // ArcEntity
            {
                Math::Point3 center;
                double radius = 0.0;
                double startAngle = 0.0;
                double endAngle = 0.0;
                file.read((char*)&center, sizeof(Math::Point3));
                file.read((char*)&radius, sizeof(double));
                file.read((char*)&startAngle, sizeof(double));
                file.read((char*)&endAngle, sizeof(double));

                EntityAttr attr;
                file.read((char*)&attr.Color, sizeof(Math::Color4));
                file.read((char*)&attr.LayerId, sizeof(LayerID));
                file.read((char*)&attr.LineType, sizeof(LineType));
                file.read((char*)&attr.LineWidth, sizeof(float));
                file.read((char*)&attr.Visible, sizeof(bool));

                auto entity = std::make_unique<ArcEntity>(id, center, radius, startAngle, endAngle);
                entity->SetAttr(attr);
                scene.AddEntity(std::move(entity));
            }
            else if (type == 6) // EllipseEntity
            {
                Math::Point3 center;
                double rx = 0.0;
                double ry = 0.0;
                double rotation = 0.0;
                file.read((char*)&center, sizeof(Math::Point3));
                file.read((char*)&rx, sizeof(double));
                file.read((char*)&ry, sizeof(double));
                file.read((char*)&rotation, sizeof(double));

                EntityAttr attr;
                file.read((char*)&attr.Color, sizeof(Math::Color4));
                file.read((char*)&attr.LayerId, sizeof(LayerID));
                file.read((char*)&attr.LineType, sizeof(LineType));
                file.read((char*)&attr.LineWidth, sizeof(float));
                file.read((char*)&attr.Visible, sizeof(bool));

                auto entity = std::make_unique<EllipseEntity>(id, center, rx, ry, rotation);
                entity->SetAttr(attr);
                scene.AddEntity(std::move(entity));
            }
            else if (type == 7) // PolylineEntity
            {
                uint32_t pointCount = 0;
                file.read((char*)&pointCount, sizeof(uint32_t));

                std::vector<Math::Point3> points(pointCount);
                file.read((char*)points.data(), sizeof(Math::Point3) * pointCount);

                uint32_t bulgeCount = 0;
                file.read((char*)&bulgeCount, sizeof(uint32_t));

                std::vector<double> bulges(bulgeCount);
                if (bulgeCount > 0)
                    file.read((char*)bulges.data(), sizeof(double) * bulgeCount);

                EntityAttr attr;
                file.read((char*)&attr.Color, sizeof(Math::Color4));
                file.read((char*)&attr.LayerId, sizeof(LayerID));
                file.read((char*)&attr.LineType, sizeof(LineType));
                file.read((char*)&attr.LineWidth, sizeof(float));
                file.read((char*)&attr.Visible, sizeof(bool));

                auto entity = std::make_unique<PolylineEntity>(id, std::move(points), std::move(bulges));
                entity->SetAttr(attr);
                scene.AddEntity(std::move(entity));
            }
        }

        printf("Opened: %s (%d entities)\n", filePath, count);
    }

    void DocumentManager::Save()
    {
        if (!m_active) return; // ← 判空移到最前面

        std::filesystem::path p = m_active->GetName();
        if (p.extension() != ".dwg")
            p.replace_extension(".dwg");

        std::filesystem::path outputPath = std::filesystem::current_path() / p; // ← 用修正后的 p

            m_active->SetPath(outputPath.string());
            m_active->Save();
        
    }

    void DocumentManager::SaveAs()
    {
        if (!m_active)
            return;

        char filePath[MAX_PATH] = {};

        // 预填当前文件名
        std::string currentName = m_active->GetName();
        // 去掉已有的 .dwg / .DWG 后缀
        static const std::string ext = ".dwg";
        if (m_active->GetName().size() > ext.size()) {
            std::string tail = m_active->GetName().substr(m_active->GetName().size() - ext.size());
            std::transform(tail.begin(), tail.end(), tail.begin(), ::tolower);
            if (tail == ext)
                currentName = currentName.substr(0, currentName.size() - ext.size());
        }

        m_active->GetName().copy(filePath, m_active->GetName().size());
        currentName.copy(filePath, currentName.size());

        OPENFILENAMEA ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFilter = "DWG Files (*.dwg)\0*.dwg\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = filePath;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = "Save As";
        ofn.lpstrDefExt = "dwg";
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

        if (!GetSaveFileNameA(&ofn))
            return; // 用户取消了

        m_active->SaveAs(filePath);
    }

    void DocumentManager::SaveAll()
    {
        for (auto& doc : m_docs)
        {
            doc->Save();
        }
    }

    void DocumentManager::Undo() const
    {
        GetActive()->Undo(); 
    }

    void DocumentManager::Redo() const
    {
        GetActive()->Redo();
    }


    void DocumentManager::Paste()
    {
        printf("Paste\n"); 
    }

    void DocumentManager::CopySelected() 
    {
        printf("Copy Selected\n"); 
    }

    std::string DocumentManager::GenerateUniqueName()
    {
        int index = 0;

        while (true)
        {
            std::string name = "Untitled";
            if (index > 0)
                name += " " + std::to_string(index);

            bool exists = false;
            for (auto& d : m_docs)
            {
                if (d->GetName() == name)
                {
                    exists = true;
                    break;
                }
            }

            if (!exists)
                return name;

            index++;
        }
    } 

}