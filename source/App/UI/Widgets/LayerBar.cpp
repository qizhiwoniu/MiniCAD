#include "LayerBar.h"
#include <imgui.h> 
#include "App/Document/Document.h"
#include <imgui_internal.h>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <cstring>
namespace YGsoftware
{
    const char* LayerBar::GetName() const
    {
        return "图层栏";
    }
    void LayerBar::OnRender(Document& document)
    {
        if (!g_show_LayerBarPanel)
            return;

        ImGui::Begin(GetName(), &g_show_LayerBarPanel);
        {
            auto& lm = document.GetLayerManager();

            // --- 新建图层 ---
            static char newLayerName[128] = "New Layer";
            ImGui::InputText("##new_layer_name", newLayerName, (size_t)128);
            ImGui::SameLine();
            if (ImGui::Button("Add Layer"))
            {
                std::string name(newLayerName);
                if (!name.empty())
                {
                    lm.AddLayer(name);
                }
            }

            ImGui::Spacing();

            // 获取并按名称排序图层列表，保证每次显示顺序稳定且友好
            auto ids = lm.GetAllLayerIDs();
            std::sort(ids.begin(), ids.end(), [&](LayerID a, LayerID b)
            {
                const Layer* A = lm.GetLayer(a);
                const Layer* B = lm.GetLayer(b);
                if (!A || !B) return a < b;
                return A->GetName() < B->GetName();
            });

            // 为每个图层维护一个可编辑的名字缓冲区（在多帧间持久化）
            static std::unordered_map<LayerID, std::vector<char>> nameBuffers;

            ImGui::Columns(6, "layer_cols", true);
            ImGui::Separator();
            ImGui::TextUnformatted("Active"); ImGui::NextColumn();
            ImGui::TextUnformatted("Color");  ImGui::NextColumn();
            ImGui::TextUnformatted("Name");   ImGui::NextColumn();
            ImGui::TextUnformatted("Visible");ImGui::NextColumn();
            ImGui::TextUnformatted("Locked"); ImGui::NextColumn();
            ImGui::TextUnformatted("Actions");ImGui::NextColumn();
            ImGui::Separator();

            for (LayerID id : ids)
            {
                Layer* layer = lm.GetLayer(id);
                if (!layer) continue;

                ImGui::PushID((int)id);

                // Active 切换（单选）
                bool isActive = (lm.GetActiveLayerID() == id);
                if (ImGui::RadioButton("##active", isActive))
                {
                    lm.SetActiveLayerID(id);
                }
                ImGui::NextColumn();

                // 颜色编辑
                const auto& c = layer->GetColor();
                float col[4] = { c.x, c.y, c.z, c.w };
                if (ImGui::ColorEdit4("##color", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
                {
                    layer->SetColor(DirectX::XMFLOAT4{ col[0], col[1], col[2], col[3] });
                }
                ImGui::NextColumn();

                // 名称编辑（保证缓冲区足够大）
                auto& buf = nameBuffers[id];
                if (buf.empty())
                {
                    buf.resize(256);
                    std::strncpy(buf.data(), layer->GetName().c_str(), buf.size());
                    buf[buf.size() - 1] = '\0';
                }
                if (ImGui::InputText("##name", buf.data(), buf.size(), ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    layer->SetName(std::string(buf.data()));
                }
                ImGui::NextColumn();

                // 可见性
                bool vis = layer->IsVisible();
                if (ImGui::Checkbox("##vis", &vis))
                    layer->SetVisible(vis);
                ImGui::NextColumn();

                // 锁定
                bool locked = layer->IsLocked();
                if (ImGui::Checkbox("##lock", &locked))
                    layer->SetLocked(locked);
                ImGui::NextColumn();

                // 操作：删除（默认图层不可删除）
                if (id != Layer::DefaultLayerID)
                {
                    std::string delPopup = "ConfirmDelete-" + std::to_string(id);
                    if (ImGui::Button("Delete"))
                    {
                        ImGui::OpenPopup(delPopup.c_str());
                    }
                    if (ImGui::BeginPopupModal(delPopup.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                    {
                        ImGui::Text("Delete layer '%s'?", layer->GetName().c_str());
                        ImGui::Separator();
                        if (ImGui::Button("Yes", ImVec2(120, 0)))
                        {
                            lm.RemoveLayer(id);
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("No", ImVec2(120, 0)))
                        {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                }
                else
                {
                    ImGui::TextDisabled("Default");
                }

                ImGui::NextColumn();

                ImGui::PopID();
            }

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::End();
        }
    }
}
