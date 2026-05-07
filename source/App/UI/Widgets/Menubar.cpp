#include "Menubar.h"
#include <imgui.h>
#include "App/Document/Document.h"
#include "PropertyPanel.h"
#include "LayerBar.h"

namespace YGsoftware
{
    const char* Menubar::GetName() const
    {
        return "Menubar";
    }
  
    bool g_show_PropertyPanel = false;
    bool g_show_ToolBarPanel = true;
    bool g_show_StatusBar_panel = true;
    bool g_show_LayerBarPanel = false;

    float Menubar::s_bottomY = 0.0f;

    void Menubar::OnRender(Document& document)
    {
        // 不能用 Begin
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("文件(F)"))
            {
                ImGui::MenuItem("新建(N)", "Ctrl+N", false, false);{}
                ImGui::MenuItem("打开(O)", "Ctrl+O", false, false); {}
                if (ImGui::BeginMenu("最近打开(R)", false))
                {
                    ImGui::MenuItem("Hello");
                    ImGui::MenuItem("Sailor");
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("保存(S)", "Ctrl+S", false, false)) {}
                if (ImGui::MenuItem("另存为(A)..", "Ctrl+S", false, false)) {}
                ImGui::Separator();
                if (ImGui::MenuItem("导入(I)..", "Ctrl+I", false, false)) {}
                if (ImGui::MenuItem("导出(E)..", "Ctrl+E", false, false)) {}
                ImGui::Separator();
                if (ImGui::MenuItem("退出(X)", "Alt+F4", false, false)) {}
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("编辑(E)"))
            {
                if (ImGui::MenuItem("撤销(U)", "Ctrl+Z")) { document.Undo(); }
                if (ImGui::MenuItem("重做(R)", "Ctrl+Y")) { document.Redo(); }
                ImGui::Separator();
                ImGui::MenuItem("剪切(T)", "Ctrl+X", false, false);
                ImGui::MenuItem("复制(C)", "Ctrl+C", false, false);
                ImGui::MenuItem("粘贴(P)", "Ctrl+V", false, false);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("视图(V)"))
            {
                ImGui::MenuItem("场景(S)", "默认");
                ImGui::Separator();
                ImGui::MenuItem("特性(P)", "Ctrl+P", &g_show_PropertyPanel); 
                ImGui::MenuItem("工具栏(T)", "Ctrl+T", &g_show_ToolBarPanel);
                ImGui::MenuItem("图层栏(L)", "Ctrl+L", &g_show_LayerBarPanel);
                ImGui::MenuItem("状态栏(E)", "Ctrl+E", &g_show_StatusBar_panel);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("工具(T)"))
            {
                bool showGrid = document.GetEditor().IsGridShown();
                if (ImGui::MenuItem("Grid(G)", "G", &showGrid))
                {
                    document.GetEditor().SetShowGrid(showGrid);
                }

                bool showAxis = document.GetEditor().IsAxisShown();
                if (ImGui::MenuItem("Axis(A)", "A", &showAxis))
                {
                    document.GetEditor().SetShowAxis(showAxis);
                }
                bool showGizmo = document.GetEditor().IsGizmoShown();
                if (ImGui::MenuItem("Gizmo(G)", "G", &showGizmo))
                {
                    document.GetEditor().SetShowGizmo(showGizmo);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Guide(G)", "Ctrl+I", false, false))
                {
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("扩展(E)"))
            {
                if (ImGui::MenuItem("Demo", "", false, false)) {} 
                ImGui::Separator();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("帮助(H)"))
            {
                if (ImGui::MenuItem("在线(O)", "", false, false)) {}
                if (ImGui::MenuItem("CHM(H)", "", false, false)) {} 
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }
}