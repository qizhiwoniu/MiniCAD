#include "UIManager.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_internal.h>
#include "App/UI/Widgets/PropertyPanel.h"
#include "App/UI/Widgets/Menubar.h"
#include "App/UI/Widgets/StatusBar.h"
#include "App/UI/Widgets/ToolBar.h"
#include "App/UI/Widgets/LayerBar.h"
#include "App/UI/Widgets/Ribbon.h"
#include "App/UI/Widgets/RibbonTab.h"
#include "App/Document/DocumentManager.h"
#include <memory>
namespace YGsoftware
{
    bool UIManager::Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
    {
        m_imgui = std::make_unique<ImGuiLayer>();

        if (!m_imgui->Init(hwnd, device, context))
            return false;
        //m_widgets.push_back(std::make_unique<Ribbon>());
        m_ribbon = std::make_unique<Ribbon>();           // Ribbon
        auto tab = m_ribbon->AddTab("主页");
        m_ribbon->AddTab("文件");
        m_ribbon->AddTab("视图");
        //m_menubar = std::make_unique<Menubar>();          // 菜单栏
        m_widgets.push_back(std::make_unique<Menubar>());
        m_widgets.push_back(std::make_unique<PropertyPanel>());  // 添加属性面板 
        m_widgets.push_back(std::make_unique<StatusBar>());      // 状态栏
        m_widgets.push_back(std::make_unique<ToolBar>());        // 工具栏
        m_widgets.push_back(std::make_unique<LayerBar>());       // 图层栏 


        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF(
            "C:/Windows/Fonts/msyh.ttc",
            16.0f,
            nullptr,
            io.Fonts->GetGlyphRangesChineseFull()
        );

        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding = 4.0f;
        style.FrameRounding = 3.0f;
        style.ChildRounding = 3.0f;

        style.WindowPadding = ImVec2(6, 6);
        style.FramePadding = ImVec2(4, 2);
        style.ItemSpacing = ImVec2(4, 4);

        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        auto& colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.1f, 0.1f, 1.0f };

        colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f };
        colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.3f, 0.3f, 1.0f };
        colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };

        colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f };
        colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.3f, 0.3f, 1.0f };
        colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };

        colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f };
        colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.3f, 0.3f, 1.0f };
        colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };

        colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };
        colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.38f, 0.38f, 1.0f };
        colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.28f, 0.28f, 1.0f };
        colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f };

        colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };
        colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };

        return true;
    }

    void UIManager::Shutdown()
    {
        m_imgui->Shutdown();
    }

    void UIManager::BeginFrame()
    {
        m_imgui->Begin();
    }

    void UIManager::EndFrame()
    {
        m_imgui->End();
    }
    ImGuiWidgetBase* UIManager::FindWidget(const std::string& id)
    {
        for (auto& w : m_widgets)
        {
            if (id == w->GetID())
                return w.get();
        }
        return nullptr;
    }

    // =========================================================
    // 主渲染入口 
    // =========================================================
    void UIManager::Render(DocumentManager& dm)
    {
        DrawDockSpace();
        DrawDocumentTabs(dm);
        // 先渲染菜单栏
        //m_menubar->OnRender(dm);   // 建议把 Menubar 单独存指针

        // 用菜单栏真实底部 Y 定位 Ribbon，彻底消除缝隙
        ImGuiViewport* vp = ImGui::GetMainViewport();
        const float ribbonY = Menubar::s_bottomY;
        const float ribbonH = 72.0f;//72

        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, ribbonY));
        ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, ribbonH));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("Ribbon", nullptr,
            ImGuiWindowFlags_NoScrollWithMouse |
            //ImGuiWindowFlags_AlwaysVerticalScrollbar |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar(2);

        m_ribbon->OnRender(dm);
        
        ImGui::End();

        if (auto* doc = dm.GetActive())
        {
            for (const auto& widget : m_widgets)
            {
                if (!widget) continue;
                widget->OnRender(*doc);
            }
        }
    }
    void  UIManager::DrawDocumentTabs(DocumentManager& dm)
    {
        ImGui::Begin("Document");
        if (!ImGui::BeginTabBar("Documents##Main"))
        {
            ImGui::End();
            return;
        }

        auto& docs = dm.GetAll();
        Document* active = dm.GetActive();

        for (auto& docPtr : docs)
        {
            Document* doc = docPtr.get();
            bool open = true;

            ImGui::PushID(doc);

            std::string label = doc->GetName();
            if (doc->IsDirty())
                label += " *";

            if (ImGui::BeginTabItem(label.c_str(), &open))
            {
                if (doc != active)
                    dm.SetActive(doc);

                ImGui::EndTabItem();
            }

            ImGui::PopID();

            if (!open)
            {
                dm.Close(doc);
                if (doc == active)
                    dm.SetActive(nullptr);
                break;
            }
        }

        ImGui::EndTabBar();
        ImGui::End();
    }
    // =========================================================
    // DockSpace 
    void UIManager::DrawDockSpace()
    {
        ImGuiViewport* vp = ImGui::GetMainViewport();

        // DockRoot 从保留区域下方开始
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("DockRoot", nullptr, flags);
        ImGui::PopStyleVar();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 100));

        ImGuiID id = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::End();
    }

}