#include "App/UI/Widgets/RibbonTab.h"
#include "App/UI/Widgets/RibbonGroup.h"
#include "App/UI/UIElement.h"
#include <d3d11.h>
#include <imgui.h>

namespace YGsoftware
{
        void RibbonTab::OnRender(Document& document)
        {
            //ImGui::Text("Tab Content"); // ⭐ 调试用，确认执行了

            if (m_groups.empty())
            {
                ImGui::BeginChild("主页", ImVec2(80, 48), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

                if (ImGui::Button("Test", ImVec2(32, 32)))//(72, 40)))
                {
                    printf("Button Clicked!\n");
                }ImGui::SameLine();
                if (ImGui::Button("btn", ImVec2(32, 32)))//(72, 40)))
                {
                    printf("Button Clicked!\n");
                }
                ImGui::EndChild();
                ImGui::BeginChild("视图", ImVec2(48, 48), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

                if (ImGui::Button("左视图", ImVec2(32, 32)))//(72, 40)))
                {
                    printf("Button Clicked!\n");
                }

                ImGui::EndChild();

                return;
            }

            for (auto& group : m_groups)
            {
                group->OnRender(document);
                ImGui::SameLine();
            }
        }
        RibbonGroup* RibbonTab::AddGroup(const std::string & name)
        {
            auto g = std::make_unique<RibbonGroup>();
            RibbonGroup* ptr = g.get();
            m_groups.push_back(std::move(g));
            return ptr;
        }
    }
