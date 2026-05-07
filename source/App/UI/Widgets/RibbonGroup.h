#pragma once
#include <string>
#include <imgui.h>
#include "App/Document/Document.h"
namespace YGsoftware
{
  

    class RibbonGroup
    {
    public:
        RibbonGroup() = default;
        ~RibbonGroup() = default;

        void SetName(const std::string& name)
        {
            m_name = name;
        }

        const std::string& GetName() const
        {
            return m_name;
        }

        void OnRender(Document& document)
        {
            ImGui::Text("%s", m_name.c_str());

            // 👉 测试按钮
            if (ImGui::Button("Btn"))
            {
            }
        }

    private:
        std::string m_name;
    };
}