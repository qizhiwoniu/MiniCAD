#pragma once
#include <vector>
#include <memory>
#include <string>

#include "ImGuiWidgetBase.h"
#include "RibbonTab.h"
#include "App/Document/Document.h"
#include "App/Document/DocumentManager.h"
namespace YGsoftware
{  
    class Ribbon
    {
    public:
        void OnRender(DocumentManager& dm);
        const char* GetName() const { return "Ribbon"; }

        // RibbonTab* AddTab(const std::string& name);
        std::shared_ptr<RibbonTab> AddTab(const std::string& name);
    private:
        //std::vector<std::unique_ptr<RibbonTab>> m_tabs;
        std::vector<std::shared_ptr<RibbonTab>> m_tabs;
        int m_activeTab = 0;
    };
}