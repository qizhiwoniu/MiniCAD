#include "PropertyPanel.h"
#include <imgui.h> 
#include "App/Document/Document.h"
#include "Core/Entity/LineEntity.hpp"
#include "App/Command/UpdateLineEntityCommand.h"
#include "App/UI/Widgets/PropertySubPanelLine.h"
namespace YGsoftware
{
    PropertyPanel::PropertyPanel()
    {
        m_subPanels[typeid(LineEntity)] = std::make_unique<PropertySubPanelLine>(); 
    }

    const char* PropertyPanel::GetName() const { return "特性"; }
     
    void PropertyPanel::OnRender(Document& document)
    {
        // If the global visibility flag is false, skip rendering.
        if (!g_show_PropertyPanel)
            return;

        // Pass the pointer to the global flag so ImGui will set it to false
        // when the user closes the window using the titlebar close button.
        if (!ImGui::Begin(GetName(), &g_show_PropertyPanel))
        {
            ImGui::End();
            return;
        }

        auto* object = document.GetEditor().GetPrimarySelectedObject();
        if (!object)
        {
            ImGui::Text("未选择对象");
            ImGui::End();
            return;
        }

        // 按实际运行时类型分发 
        auto it = m_subPanels.find(typeid(*object));

        if (it != m_subPanels.end())
            it->second->OnRender(object, document);
        else
            ImGui::Text("未知对象类型");

        ImGui::End();
    }
}