#include "DocumentManager.h"
#include "Document.h"
#include "Core/Entity/PointEntity.hpp"
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
                XMFLOAT3 start, end;
                bool isSegment;
                file.read((char*)&start, sizeof(XMFLOAT3));
                file.read((char*)&end, sizeof(XMFLOAT3));
                file.read((char*)&isSegment, sizeof(bool));

                EntityAttr attr;
                file.read((char*)&attr.Color, sizeof(XMFLOAT4));
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
                XMFLOAT3 pos;
                file.read((char*)&pos, sizeof(XMFLOAT3));

                EntityAttr attr;
                file.read((char*)&attr.Color, sizeof(XMFLOAT4));
                file.read((char*)&attr.LayerId, sizeof(LayerID));
                file.read((char*)&attr.LineType, sizeof(LineType));
                file.read((char*)&attr.LineWidth, sizeof(float));
                file.read((char*)&attr.Visible, sizeof(bool));

                auto entity = std::make_unique<PointEntity>(id, pos);
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
        std::string currentName = m_active->GetName() + ".dwg";
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