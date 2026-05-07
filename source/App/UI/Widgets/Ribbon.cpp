    #include "Ribbon.h"
    #include "RibbonTab.h"
    #include "App/Document/DocumentManager.h"
    #include <imgui.h>

    namespace YGsoftware
    {
        void Ribbon::OnRender(DocumentManager& dm)
        {
            ImGui::BeginChild("RibbonBar", ImVec2(0, 72), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            // =========================
            // 1. 内容区（上面）
            // =========================
            ImGui::BeginChild("RibbonContent", ImVec2(0, 40), true,ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            auto* doc = dm.GetActive();
            if (doc && m_activeTab >= 0 && m_activeTab < m_tabs.size())
            {
                m_tabs[m_activeTab]->OnRender(*doc);
            }

            ImGui::EndChild();

            //ImGui::Separator();

            // =========================
            // 2. Tab 区（下面）
            // =========================

            float availableWidth = ImGui::GetContentRegionAvail().x;
            float x = 0.0f;

            for (size_t i = 0; i < m_tabs.size(); ++i)
            {
                std::string name = m_tabs[i]->GetName();

                // ⚠️ 关键：避免按钮无限拉伸
                ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
                float btnWidth = textSize.x + 20.0f;

                if (i > 0 && x + btnWidth > availableWidth)
                {
                    x = 0;
                    ImGui::NewLine();   // 👉 强制换行
                }
                else if (i > 0)
                {
                    ImGui::SameLine();
                }

                x += btnWidth;

                bool selected = (i == m_activeTab);

                if (selected)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.4f, 1));

                if (ImGui::Button(name.c_str()))
                    m_activeTab = (int)i;

                if (selected)
                    ImGui::PopStyleColor();
            }
            ImGui::EndChild();
        }
        std::shared_ptr<RibbonTab> Ribbon::AddTab(const std::string & name)
        {
            auto tab = std::make_shared<RibbonTab>(name);
            m_tabs.push_back(tab);
            return tab;
        }
    }