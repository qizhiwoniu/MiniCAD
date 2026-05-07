#pragma once
#include <string>
#include <vector>
#include <memory>
#include "App/UI/UIElement.h"
#include "App/UI/Widgets/RibbonGroup.h"
#include <d3d11.h>
#include "App/Document/Document.h"
namespace YGsoftware
{
   
    class RibbonTab : public UIElement
    {
    public:
        // =========================
        // 名称
        // =========================
        void SetName(const std::string& name)
        {
            m_name = name;
        }

        const std::string& GetName() const
        {
            return m_name;
        }
        RibbonTab(const std::string& name)   // ⭐ 加这一行
            : m_name(name)
        {}
        // =========================
        // 渲染
        // =========================
        void OnRender(Document& document);
        RibbonGroup* AddGroup(const std::string& name);

    private:
        std::vector<std::unique_ptr<RibbonGroup>> m_groups;
        std::string m_name;
        bool m_selected = false;
    };
}