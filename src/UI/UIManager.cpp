#include "UIManager.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "Document/Document.h"
#include "Document/DocumentManager.h"
#include "Editor/Input/InputEvent.h"
#include "Scene/LayerManager.h"
#include "Scene/Layer.h"
#include "Core/Entity/Entity.hpp"
#include <memory>
#include <vector>
#include <filesystem>
#include <utility>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h> 
#include "pch.h" 
#include <functional>
#include "../../version.h" 

namespace MiniCAD
{ 
    namespace
    {
        constexpr float kToolbarHeight   = 38.f;   // 工具栏高度
        constexpr float kToolBtnSize     = 32.f;   // 工具按钮尺寸
        constexpr float kStatusBarHeight = 26.f;   // 状态栏高度

        // 工具元数据
        struct ToolMeta 
        { 
            Tool        id; 
            const char* icon;  
            const char* tooltip;
            std::function<void(DocumentManager&)> onActivate;
        };
        inline  ToolMeta kTools[] =
        {
            { Tool::Select,     "Cursor",  "选择 (Esc)"   , [](DocumentManager& dm) {}   },
            /*---------------------------------------------*/
            { Tool::Line,       "Line",    "直线 (L)"      ,[](DocumentManager& dm) {dm.GetActive()->GetEditor().StartLineTool(); }  },
            { Tool::Circle,     "Circle",  "圆 (C)"        ,[](DocumentManager& dm) {dm.GetActive()->GetEditor().StartCircleTool(); }  },
            { Tool::Rectangle,  "Rect",    "矩形 (R)"      ,[](DocumentManager& dm) {dm.GetActive()->GetEditor().StartRectangleTool(); }  },
            { Tool::Arc,        "Arc",     "圆弧 (A)"      ,[](DocumentManager& dm) {dm.GetActive()->GetEditor().StartArcTool(); }  },
            { Tool::Ellipse,    "Ellipse", "椭圆 (E)"      ,[](DocumentManager& dm) {dm.GetActive()->GetEditor().StartEllipseTool(); }  },
            { Tool::Polyline,   "Pline",   "多段线 (Pl)"   ,[](DocumentManager& dm) {dm.GetActive()->GetEditor().StartPolylineTool(); }  },
            { Tool::Spline,     "Spline",  "样条曲线 (SPL)",[](DocumentManager& dm) {dm.GetActive()->GetEditor().StartSplineTool(); }   },
            /*---------------------------------------------*/ 
            { Tool::Copy,       "Copy",    "复制 (co)"     ,[](DocumentManager& dm) {dm.GetActive()->GetEditor().StartCopyTool(); }},
            { Tool::Move,       "Move",    "移动 (mv)"     ,[](DocumentManager& dm) {dm.GetActive()->GetEditor().StartMoveTool(); }},
            { Tool::Mirror,     "Mirror",  "镜像 (mi)"     ,[](DocumentManager& dm) {dm.GetActive()->GetEditor().StartMirrorTool(); }},
            { Tool::Rotate,     "Rotate",  "旋转 (R)"      ,[](DocumentManager& dm) {dm.GetActive()->GetEditor().StartRotateTool(); }}, 
            /*---------------------------------------------*/
            { Tool::Layer,      "LayerClose","图层 (L)"    ,[](DocumentManager& dm) {}},
            { Tool::AxisGrid,   "Axisgrid","一键轴网 (Ax)" ,[](DocumentManager& dm) {}},
            /*---------------------------------------------*/ 
            { Tool::Undo,       "Undo",    "撤销"          ,[](DocumentManager& dm) {dm.Undo();}},
            { Tool::Redo,       "Redo",    "重做"          ,[](DocumentManager& dm) {dm.Redo();}},
        };
    }
   

  
    // ── 工具函数 ────────────────────────────────────────────
    static ImVec2 RectCenter(ImVec2 min, ImVec2 size)
    {
        return ImVec2(min.x + size.x * 0.5f, min.y + size.y * 0.5f);
    }

    static void DrawMinimizeIcon(ImDrawList* dl, ImVec2 center, float size, ImU32 col)
    {
        float half = size * 0.5f;
        dl->AddLine(
            ImVec2(center.x - half, center.y ),
            ImVec2(center.x + half, center.y ),
            col, 1.2f
        );
    }

    static void DrawMaximizeIcon(ImDrawList* dl, ImVec2 center, float size, ImU32 col)
    {
        float half = size * 0.5f;
        dl->AddRect(
            ImVec2(center.x - half, center.y - half),
            ImVec2(center.x + half, center.y + half),
            col, 0.f, 0, 1.2f
        );
    }

    static void DrawRestoreIcon(ImDrawList* dl, ImVec2 center, float size, ImU32 col)
    {
        float h   = size * 0.40f;
        float off = size * 0.10f;

        // ── 后面的矩形（右上角偏移，只画三条边，左下角被前矩形遮住不画）
        ImVec2 b0 = ImVec2(center.x - h + off, center.y - h - off); // 左上
        ImVec2 b1 = ImVec2(center.x + h + off, center.y - h - off); // 右上
        ImVec2 b2 = ImVec2(center.x + h + off, center.y + h - off); // 右下
   
        dl->AddLine(b0, b1, col, 1.2f);   // 上边
        dl->AddLine(b1, b2, col, 1.2f);   // 右边 

        // ── 前面的矩形（左下角偏移，完整画四条边）
        ImVec2 f0 = ImVec2(center.x - h - off, center.y - h + off); // 左上
        ImVec2 f1 = ImVec2(center.x + h - off, center.y - h + off); // 右上
        ImVec2 f2 = ImVec2(center.x + h - off, center.y + h + off); // 右下
        ImVec2 f3 = ImVec2(center.x - h - off, center.y + h + off); // 左下

        dl->AddLine(f0, f1, col, 1.2f);
        dl->AddLine(f1, f2, col, 1.2f);
        dl->AddLine(f2, f3, col, 1.2f);
        dl->AddLine(f3, f0, col, 1.2f);
    }

    static void DrawCloseIcon(ImDrawList* dl, ImVec2 center, float size, ImU32 col)
    {
        // 像素对齐，消除亚像素偏移导致的不对称
        float cx = center.x;
        float cy = center.y;
         
        dl->AddLine(ImVec2(cx - 5.0, cy - 5.0), ImVec2(cx + 5.0, cy + 5.4), col, 1.2f);
        dl->AddLine(ImVec2(cx + 5.0, cy - 5.4), ImVec2(cx - 5.0, cy + 5.0), col, 1.2f);
    }

    static void DrawDropdownIcon(ImDrawList* dl, ImVec2 center, float size, ImU32 col)
    {
        float half = size * 0.5f;

        ImVec2 p1 = ImVec2(center.x - half, center.y - half * 0.3f);
        ImVec2 p2 = ImVec2(center.x + half, center.y - half * 0.3f);
        ImVec2 p3 = ImVec2(center.x, center.y + half);

        dl->AddTriangleFilled(p1, p2, p3, col);
    }

    static void DrawPetalLogo(ImDrawList* dl, ImVec2 center, float radius, int petalCount = 5, float amplitude = 0.12f, int segments = 120)
    {
        constexpr ImU32 col = IM_COL32(80, 180, 60, 255);   // 草绿

        std::vector<ImVec2> pts;
        pts.reserve(segments + 1);
        for (int i = 0; i <= segments; i++)
        {
            float a = IM_PI * 2.f * i / segments;
            float r = radius * (1.f + amplitude * sinf(petalCount * a));
            pts.push_back({ center.x + r * cosf(a), center.y + r * sinf(a) });
        }

        // ── 三角扇 ──────────────────────────────────────────
        for (int i = 0; i < segments; i++)
            dl->AddTriangleFilled(center, pts[i], pts[i + 1], col);
    }

    bool UIManager::Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
    {
        m_hwnd   = hwnd; 
		m_device = device;
        InitToolIcons();
        m_imgui  = std::make_unique<ImGuiLayer>();

        if (!m_imgui->Init(hwnd, device, context))
            return false;  

        ImGuiIO& io = ImGui::GetIO();   
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigWindowsMoveFromTitleBarOnly = true;  // 👈 加这行
        io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/msyh.ttc", 16.f, nullptr, io.Fonts->GetGlyphRangesChineseFull());  
        ImGui::StyleColorsDark();

		m_showAxisGrid = false;
        
        return true;
    }     
    void UIManager::Shutdown()  { m_imgui->Shutdown(); }
    void UIManager::BeginFrame(){ m_imgui->Begin(); }
    void UIManager::EndFrame()  { m_imgui->End(); }
     
    void UIManager::Render(DocumentManager& dm)
    {    
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar            |
                                 ImGuiWindowFlags_NoCollapse            |
                                 ImGuiWindowFlags_NoResize              |
                                 ImGuiWindowFlags_NoMove                |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoNavFocus            |
                                 ImGuiWindowFlags_NoBackground          | 
                                 ImGuiWindowFlags_MenuBar               |
                                 ImGuiWindowFlags_NoScrollbar           |      // 不显示滚动条
                                 ImGuiWindowFlags_NoScrollWithMouse     ;      // 禁止滚动行为
          
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(10.0, 6.f)); // 菜单栏高度 
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg,    ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_Border,       ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.f, 0.f, 0.f, 0.f)); 

        ImGui::Begin("MiniCAD", nullptr, flags);

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(3);
        // ── 1. 菜单栏（含最小化/最大化/关闭） ──────────────────
        DrawMenubar(dm);

        // ── 2. 工具栏 ────────────────────────────────────────────
        DrawToolbar(dm);

        // ── 3. 绘图区（剩余高度 - 状态栏） ──────────────────────
        { 
            ImGui::BeginChild("##DocArea", ImVec2(0, -kStatusBarHeight),  false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove);
            DrawDocumentTabs(dm);
            ImGui::EndChild();
        }

        // ── 4. 状态栏 ────────────────────────────────────────────
        DrawStatusBar(dm); 

        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode); 
        ImGui::End(); 

		// 显示图层管理器和轴网管理器（如果对应标志为 true）
        ShowLayerManager(dm);

        ShowAxisgridManager(dm);
    }
  
    void UIManager::DrawMenubar(DocumentManager& dm)
    { 
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.f, 6.f)); // 菜单栏高度 
        
        if (!ImGui::BeginMenuBar())
        {
            ImGui::PopStyleVar();  
            return;
        }
           
        // ── Logo 花瓣 鼠标悬浮 缓慢旋转─────────────────────────────────────────────
        {
            const float radius = 8.f;
            const float padL   = 2.f;   // Logo 左侧留白
            const float gapR   = 12.f;  // Logo 右侧距菜单间距
            ImDrawList* dl     = ImGui::GetWindowDrawList();
            ImVec2      pos    = ImGui::GetCursorScreenPos();
            float       menuH  = ImGui::GetFrameHeight();

            ImVec2 center = { pos.x + padL + radius, pos.y + menuH * 0.5f };

            DrawPetalLogo(dl, center, radius);

            // Dummy 只占 Logo 自身宽度，右侧间距交给 SameLine
            ImGui::Dummy(ImVec2(padL + radius * 2.f, menuH));
            ImGui::SameLine(0.f, gapR);  // ← gapR 控制与菜单的距离
        }

        // ── 菜单项 ───────────────────────────────────────────────
        if (ImGui::BeginMenu("文件"))
        {
            if (ImGui::MenuItem("新建", "Ctrl+N"))         { dm.New(); }
            if (ImGui::MenuItem("打开", "Ctrl+O"))         { dm.Open(); }
            ImGui::Separator();

            if (ImGui::MenuItem("保存",     "Ctrl+S"))         { dm.Save(); dm.m_lastSaveTime = ImGui::GetTime();}
            if (ImGui::MenuItem("另存为",   "Ctrl+Shift+S"))   { dm.SaveAs(); }
            if (ImGui::MenuItem("全部保存", "Ctrl+Alt+S"))     { dm.SaveAll(); }

            ImGui::Separator();
            if (ImGui::MenuItem("退出", "Alt+F4"))
                PostMessage(m_hwnd, WM_CLOSE, 0, 0);
            ImGui::EndMenu();
        } 

        //auto& editor = dm.GetActive()->GetEditor(); 

        if (ImGui::BeginMenu("编辑"))
        {
            if (ImGui::MenuItem("撤销", "Ctrl+Z"))    { dm.Undo(); }
            if (ImGui::MenuItem("重做", "Ctrl+Y"))    { dm.Redo(); }
            if (ImGui::MenuItem("粘贴(P)", "Ctrl+V")) { dm.Paste(); }
            if (ImGui::MenuItem("复制(C)", "Ctrl+C")) { dm.CopySelected(); }
            ImGui::EndMenu();   
        }

        if (ImGui::BeginMenu("修改"))
        {
            if (ImGui::MenuItem("图层", "Layer")) { m_showLayerMgr = true; }
            if (ImGui::MenuItem("阵列", "Array")) {}
            if (ImGui::MenuItem("移动", "Move")) {}
            if (ImGui::MenuItem("镜像", "Mirror")) {}
            if (ImGui::MenuItem("旋转", "Rotate")) {}
            ImGui::EndMenu();
        }
        //auto& editor = dm.GetActive()->GetEditor();
        /***auto* active = dm.GetActive();
        if (!active)
        {
            ImGui::EndMenuBar();
            ImGui::PopStyleVar();
            return;
        }
        auto& editor = active->GetEditor();***/
        if (ImGui::BeginMenu("绘图"))
        {
            auto* activeDoc = dm.GetActive();
            if (activeDoc)
            {
                auto& editor = dm.GetActive()->GetEditor();
                if (ImGui::MenuItem("直线", "Line")) { editor.StartLineTool(); }
                if (ImGui::MenuItem("点", "Point")) { editor.StartPointTool(); }
                if (ImGui::MenuItem("矩形", "Rectangle")) { editor.StartRectangleTool(); }
                if (ImGui::MenuItem("圆", "Circle")) { editor.StartCircleTool(); }
                if (ImGui::MenuItem("圆弧", "Arc")) { editor.StartArcTool(); }
                if (ImGui::MenuItem("旋转", "Rotate")) { editor.StartRotateTool(); }

                ImGui::EndMenu();
            }
        }

        if (ImGui::BeginMenu("视图"))
        {
            auto* activeDoc = dm.GetActive(); // ← 加判空
            if (activeDoc)
            {
                auto& viewport = dm.GetActive()->GetViewport();
                static bool showGrid = true, showAxis = true, showGizmo = true;
                ImGui::MenuItem("显示网格", nullptr, &showGrid); { viewport.ShowGrid(showGrid); }
                ImGui::MenuItem("显示坐标轴", nullptr, &showAxis); { viewport.ShowAxis(showAxis); }
                ImGui::MenuItem("显示Gizmo", nullptr, &showGizmo); { viewport.ShowGizmo(showGizmo); }
            }
            ImGui::EndMenu();
        }
        static bool showAbout = false;
        if (ImGui::BeginMenu("帮助"))
        {
            if (ImGui::MenuItem("关于")) { showAbout = true; }
            ImGui::EndMenu();
        }
        if (showAbout)
        {
            ImGui::OpenPopup("关于");
            showAbout = false;
        }
        // About 弹窗
        if (ImGui::BeginPopupModal("关于", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("MiniCAD");
            ImGui::Separator();

            ImGui::Text("版本: %s" , APP_VERSION_A);
            ImGui::Text("基于 Dear ImGui");
            ImGui::Text("作者:\n            Hello");
            ImGui::Text("鸣谢:\n        Qizhiwoniu\n          七只蜗牛");
            ImGui::Spacing();

            if (ImGui::Button("关闭", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    
        // ── 右侧窗口控制按钮 ─────────────────────────────────────
        {
            const float btnW = 32.f;
            const float gap = 8.f;
            const int btnCount = 4;
            const float totalW = btnW * btnCount + gap * (btnCount - 1)
                + ImGui::GetStyle().WindowPadding.x;
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - totalW);

            float   buttonsLocalX = ImGui::GetWindowWidth() - totalW;
            ImVec2  screenPos = ImGui::GetWindowPos();
            m_captionButtonsScreenX = screenPos.x + buttonsLocalX;

            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.f, 0.f, 0.f, 0.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.f, 1.f, 1.f, 0.15f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.f, 1.f, 1.f, 0.15f));

            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImU32       iconCol = IM_COL32(255, 255, 255, 255);
            const float iconSize = 10.f;
            // ── 按钮 ───────────────────────────────────────────
            ImGui::Button("##menu", ImVec2(btnW, 0.f));
            ImVec2 center = RectCenter(ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
            DrawDropdownIcon(dl, center, iconSize, iconCol);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("菜单");
            if (ImGui::IsItemClicked())
            {
                ImGui::OpenPopup("MainMenuPopup");
            }
            if (ImGui::BeginPopup("MainMenuPopup"))
            {
                if (ImGui::MenuItem("主题"))
                {
                    // TODO
                }
                if (ImGui::MenuItem("设置"))
                {
                    // TODO
                }
                ImGui::Separator();
                if (ImGui::MenuItem("退出"))
                {
                    PostMessage(m_hwnd, WM_CLOSE, 0, 0);
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine(0.f, gap);
            // ── 最小化 ───────────────────────────────────────────
            ImGui::Button("##min", ImVec2(btnW, 0.f));
            DrawMinimizeIcon(dl, RectCenter(ImGui::GetItemRectMin(), ImGui::GetItemRectSize()), iconSize, iconCol);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("最小化");
            if (ImGui::IsItemClicked()) ShowWindow(m_hwnd, SW_MINIMIZE);

            ImGui::SameLine(0.f, gap);

            // ── 最大化 / 还原 ────────────────────────────────────
            bool maximized = IsZoomed(m_hwnd);
            ImGui::Button("##max", ImVec2(btnW, 0.f));
            ImVec2 maxCenter = RectCenter(ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
            if (maximized) DrawRestoreIcon (dl, maxCenter, iconSize, iconCol);
            else           DrawMaximizeIcon(dl, maxCenter, iconSize, iconCol);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip(maximized ? "还原" : "最大化");
            if (ImGui::IsItemClicked()) ShowWindow(m_hwnd, maximized ? SW_RESTORE : SW_MAXIMIZE);

            ImGui::SameLine(0.f, gap);

            // ── 关闭 ─────────────────────────────────────────────
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.15f, 0.15f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.65f, 0.05f, 0.05f, 1.f));
            ImGui::Button("##close", ImVec2(btnW, 0.f));
            DrawCloseIcon(dl, RectCenter(ImGui::GetItemRectMin(), ImGui::GetItemRectSize()), iconSize, iconCol);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("关闭");
            if (ImGui::IsItemClicked()) ShowWindow(m_hwnd, SW_HIDE); //PostMessage(m_hwnd, WM_CLOSE, 0, 0);
            ImGui::PopStyleColor(2);

            ImGui::PopStyleColor(3);
        }

        ImGui::EndMenuBar();   
        ImGui::PopStyleVar();
    }
     
    void UIManager::DrawToolbar(DocumentManager& dm)
    {
        ImGui::BeginChild("##Toolbar", ImVec2(0.f, kToolbarHeight), false);

        float leftPadding = 3.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + leftPadding);

        const ImVec2 btnSize(kToolBtnSize, kToolBtnSize);

        // ===== 1️ 计算背景区域 =====
        ImVec2 start = ImGui::GetCursorScreenPos();
        float padding = 3.0f;

        float bgHeight = kToolBtnSize + padding * 2.0f;
        float bgWidth = ImGui::GetContentRegionAvail().x-6;

        // 占位（关键）
        ImGui::Dummy(ImVec2(bgWidth, bgHeight));

        // ===== 2️ 画背景（不会盖按钮）=====
        ImDrawList* draw = ImGui::GetWindowDrawList();

        draw->AddRectFilled(start, ImVec2(start.x + bgWidth, start.y + bgHeight), IM_COL32(40, 40, 45, 255), 6.0f);

        // ===== 3️ 回到起点，开始画按钮 =====
        ImGui::SetCursorScreenPos(ImVec2(start.x + padding, start.y + padding));

        ImGui::BeginGroup();

        // 按钮样式（透明底）
        ImVec4 btn     = ImVec4(0, 0, 0, 0);
        ImVec4 hover   = ImVec4(0.25f, 0.25f, 0.30f, 0.8f);
        ImVec4 activeC = ImVec4(0.35f, 0.55f, 0.85f, 0.9f);
        float  imVec2  = kToolBtnSize - 3.0 * 2;
        for (auto& meta : kTools)
        {
            bool active = (m_activeTool == meta.id);

            ImGui::PushStyleColor(ImGuiCol_Button,        btn);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  active ? activeC : hover); 
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
              
            ImGui::PushID(static_cast<int>(meta.id));
            
            if (ImGui::ImageButton("##icon", m_toolIcons[meta.icon], ImVec2(imVec2, imVec2)))
            {
                m_activeTool = meta.id;
                if (meta.onActivate)
                    meta.onActivate(dm);

                // 👇 加这一行
                if (meta.id == Tool::Layer)
                {
                    m_showLayerMgr = true;
                }

                if (meta.id == Tool::AxisGrid)
                {
                    m_showAxisGrid = true; 
                }
            }

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", meta.tooltip);

            ImGui::PopID(); 
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3); 
            ImGui::SameLine(); 
            // 分隔线
            if (meta.id == Tool::Select || meta.id == Tool::Spline || meta.id == Tool::Rotate || meta.id == Tool::AxisGrid)
            {
                ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
                ImGui::SameLine();
            }
        }
        
 
        if (false) //---占位保留，不渲染---
        { 
            // ===== 4️ 右侧 Undo / Redo =====
            float rightOffset = bgWidth - (btnSize.x * 2.0f + padding * 2.0f + 6.0f);
            ImGui::SameLine(rightOffset);

            ImGui::PushStyleColor(ImGuiCol_Button, btn);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, hover);

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

            if (ImGui::ImageButton("##iconUndo", m_toolIcons["Undo"], ImVec2(imVec2, imVec2)))
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("撤销 (Ctrl+Z)");

            ImGui::SameLine(0.f, 4.f);

            if (ImGui::ImageButton("##iconRedo", m_toolIcons["Redo"], ImVec2(imVec2, imVec2)))
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("重做 (Ctrl+Y)");

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
        } 

        ImGui::EndGroup();

        ImGui::EndChild();
    }
     
    void UIManager::DrawDocumentTabs(DocumentManager& dm)
    {
       
        m_viewportInput = {}; // 每帧重置（纯状态）

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        if (!ImGui::BeginTabBar("MiniCAD##Main"))
        {
            ImGui::PopStyleVar();
            return;
        }

        auto& docs       = dm.GetAll();
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
                {
                    dm.SetActive(doc);
                    active = doc;
                }

                // =========================
                // viewport layout
                // =========================
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

                ImVec2 size = ImGui::GetContentRegionAvail();
                size.x = ImMax(size.x, 1.0f);
                size.y = ImMax(size.y, 1.0f);

                docPtr->GetViewport().Resize(size.x, size.y);

                auto srv = doc->GetViewport().GetRenderTarget().GetSRV();

                ImVec2 imageMin = ImGui::GetCursorScreenPos();
                ImGui::Image(srv, size);

                // =========================
                // 交互层
                // =========================
                ImGui::SetCursorScreenPos(imageMin);
                ImGui::InvisibleButton(
                    "##DocumentViewportInput",
                    size,
                    ImGuiButtonFlags_MouseButtonLeft |
                    ImGuiButtonFlags_MouseButtonMiddle |
                    ImGuiButtonFlags_MouseButtonRight
                );

                const bool hovered = ImGui::IsItemHovered();
                const bool activeItem = ImGui::IsItemActive();

                // =========================
                // 构建 ViewportInput（仅 active doc）
                // =========================
                if (doc == dm.GetActive())
                {
                    ImGuiIO& io = ImGui::GetIO();

                    ImVec2 mouse = io.MousePos;
                    if (!ImGui::IsMousePosValid(&mouse))
                        mouse = ImVec2(0.f, 0.f);

                    ImVec2 local = { mouse.x - imageMin.x,   mouse.y - imageMin.y  };

                    // =========================
                    // viewport meta
                    // =========================
                    m_viewportInput.Valid     = true;
                    m_viewportInput.Hovered   = hovered;
                    m_viewportInput.Active    = activeItem;
                    m_viewportInput.Focused   = hovered || activeItem;

                    m_viewportInput.Size      = { size.x, size.y };
                    m_viewportInput.ScreenMin = { imageMin.x, imageMin.y };
                    m_viewportInput.ScreenMax = { imageMin.x + size.x, imageMin.y + size.y };

                    // =========================
                    // mouse
                    // =========================
                    m_viewportInput.MouseLocal = { local.x, local.y };
                    m_viewportInput.MouseDelta = {
                        local.x - m_lastLocal.x,
                        local.y - m_lastLocal.y
                    };

                    m_viewportInput.Wheel = (hovered || activeItem) ? io.MouseWheel : 0.f;

                    // =========================
                    // MouseButtons → ButtonState（关键修正）
                    // =========================
                    for (int i = 0; i < 3; ++i)
                    {
                        auto& btn = m_viewportInput.MouseButtons[i];

                        btn.Down = io.MouseDown[i];

                        btn.Pressed = hovered && io.MouseClicked[i];
                        btn.Released = io.MouseReleased[i];
                    }

                    // =========================
                    // modifiers（唯一来源）
                    // =========================
                    m_viewportInput.Modifiers = 0;
                    if (io.KeyShift) m_viewportInput.Modifiers |= (uint8_t)ModifierKey::Shift;
                    if (io.KeyCtrl)  m_viewportInput.Modifiers |= (uint8_t)ModifierKey::Ctrl;
                    if (io.KeyAlt)   m_viewportInput.Modifiers |= (uint8_t)ModifierKey::Alt;

                    // =========================
                    // keyboard
                    // =========================
                    auto setKey = [&](ImGuiKey imguiKey, KeyCode key)  
                    {
                            auto& k = m_viewportInput.Keys[(size_t)key]; 

                            k.Down     = ImGui::IsKeyDown(imguiKey); 
                            k.Pressed  = m_viewportInput.Focused &&  ImGui::IsKeyPressed(imguiKey, false); 
                            k.Released = m_viewportInput.Focused &&  ImGui::IsKeyReleased(imguiKey);
                             

                     };

                    // =========================
                    // common keys
                    // =========================
                    setKey(ImGuiKey_Escape, KeyCode::Escape);
                    setKey(ImGuiKey_Delete, KeyCode::Delete);
                    setKey(ImGuiKey_Space,  KeyCode::Space);
                    setKey(ImGuiKey_Enter,  KeyCode::Enter);

                    setKey(ImGuiKey_A, KeyCode::A);
                    setKey(ImGuiKey_B, KeyCode::B);
                    setKey(ImGuiKey_C, KeyCode::C);
                    setKey(ImGuiKey_D, KeyCode::D);
                    setKey(ImGuiKey_E, KeyCode::E);
                    setKey(ImGuiKey_F, KeyCode::F);
                    setKey(ImGuiKey_G, KeyCode::G);

                    setKey(ImGuiKey_H, KeyCode::H);
                    setKey(ImGuiKey_I, KeyCode::I);
                    setKey(ImGuiKey_J, KeyCode::J);
                    setKey(ImGuiKey_K, KeyCode::K);
                    setKey(ImGuiKey_L, KeyCode::L);
                    setKey(ImGuiKey_M, KeyCode::M);
                    setKey(ImGuiKey_N, KeyCode::N);

                    setKey(ImGuiKey_O, KeyCode::O);
                    setKey(ImGuiKey_P, KeyCode::P);
                    setKey(ImGuiKey_Q, KeyCode::Q);
                    setKey(ImGuiKey_R, KeyCode::R);
                    setKey(ImGuiKey_S, KeyCode::S);
                    setKey(ImGuiKey_T, KeyCode::T);
                    setKey(ImGuiKey_U, KeyCode::U);

                    setKey(ImGuiKey_V, KeyCode::V);
                    setKey(ImGuiKey_W, KeyCode::W);
                    setKey(ImGuiKey_X, KeyCode::X);
                    setKey(ImGuiKey_Y, KeyCode::Y);
                    setKey(ImGuiKey_Z, KeyCode::Z);

                    // =========================
                    // numbers
                    // =========================
                    setKey(ImGuiKey_0, KeyCode::Num0);
                    setKey(ImGuiKey_1, KeyCode::Num1);
                    setKey(ImGuiKey_2, KeyCode::Num2);
                    setKey(ImGuiKey_3, KeyCode::Num3);
                    setKey(ImGuiKey_4, KeyCode::Num4);
                    setKey(ImGuiKey_5, KeyCode::Num5);
                    setKey(ImGuiKey_6, KeyCode::Num6);
                    setKey(ImGuiKey_7, KeyCode::Num7);
                    setKey(ImGuiKey_8, KeyCode::Num8);
                    setKey(ImGuiKey_9, KeyCode::Num9);

                    // =========================
                    // function keys
                    // =========================
                    setKey(ImGuiKey_F1, KeyCode::F1);
                    setKey(ImGuiKey_F2, KeyCode::F2);
                    setKey(ImGuiKey_F3, KeyCode::F3);
                    setKey(ImGuiKey_F4, KeyCode::F4);
                    setKey(ImGuiKey_F5, KeyCode::F5);
                    setKey(ImGuiKey_F6, KeyCode::F6);
                    setKey(ImGuiKey_F7, KeyCode::F7);
                    setKey(ImGuiKey_F8, KeyCode::F8);
                    setKey(ImGuiKey_F9, KeyCode::F9);
                    setKey(ImGuiKey_F10, KeyCode::F10);
                    setKey(ImGuiKey_F11, KeyCode::F11);
                    setKey(ImGuiKey_F12, KeyCode::F12);

                    // =========================
                    // modifiers
                    // =========================
                    setKey(ImGuiKey_LeftShift, KeyCode::LShift);
                    setKey(ImGuiKey_RightShift, KeyCode::RShift);

                    setKey(ImGuiKey_LeftCtrl, KeyCode::LCtrl);
                    setKey(ImGuiKey_RightCtrl, KeyCode::RCtrl);

                    setKey(ImGuiKey_LeftAlt, KeyCode::LAlt);
                    setKey(ImGuiKey_RightAlt, KeyCode::RAlt);



                    // =========================
                    // cursor
                    // =========================
                    if (hovered || activeItem)
                        ImGui::SetMouseCursor(ImGuiMouseCursor_None);

                    if (hovered || activeItem)
                        m_lastLocal = local;
                    else
                        m_viewportInput.MouseDelta = { 0.f, 0.f };

                }

                ImGui::PopStyleVar(2);
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
        ImGui::PopStyleVar();
    } 

    void UIManager::ShowLayerManager(DocumentManager& dm)
    {
        if (!m_showLayerMgr) return;

        Document* doc = dm.GetActive();
        if (!doc) { m_showLayerMgr = false; return; }

        LayerManager& lm = doc->GetLayerManager();
        EditorContext& editor = doc->GetEditor();

        ImGui::SetNextWindowPos(ImVec2(200, 200), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(440, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(300, 250), ImVec2(700, 800));

        std::string title = "图层管理器 - " + doc->GetName();
        if (!ImGui::Begin(title.c_str(), &m_showLayerMgr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking))
        {
            ImGui::End();
            return;
        }

        // ── 工具栏 ──────────────────────────────────────
        if (ImGui::Button("  +  新建  "))
        {
            static int counter = 1;
            char buf[32];
            snprintf(buf, sizeof(buf), "图层 %d", counter++);
            lm.AddLayer(buf);
        }
        ImGui::SameLine();

		
        LayerID activeID = lm.GetActiveLayerID(); // 确保有默认图层;
        bool canDelete = (activeID != Layer::DefaultLayerID);

        if (!canDelete) ImGui::BeginDisabled();
        if (ImGui::Button("  删除  "))
        {
            lm.RemoveLayer(activeID);
            lm.SetActiveLayerID(Layer::DefaultLayerID);
            lm.SetActiveLayerID(Layer::DefaultLayerID);
        }
        if (!canDelete) ImGui::EndDisabled();

        ImGui::Separator();

        // ── 表头 ────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::Text("  %-6s %-6s %-8s %s", "可见", "锁定", "颜色", "图层名");
        ImGui::PopStyleColor();
        ImGui::Separator();

        // ── 图层列表 ────────────────────────────────────
        float listHeight = -ImGui::GetFrameHeightWithSpacing() - 6.f;
        ImGui::BeginChild("##LayerList", ImVec2(0.f, listHeight), true);

        auto ids = lm.GetAllLayerIDs();
        std::sort(ids.rbegin(), ids.rend()); // 倒序，新图层在上

        for (LayerID id : ids)
        {
            Layer* layer = lm.GetLayer(id);
            if (!layer) continue;

            ImGui::PushID((int)id);

            bool isActive = (id == lm.GetActiveLayerID());

            // 激活行高亮
            if (isActive)
            {
                ImVec2 rowMin = ImGui::GetCursorScreenPos();
                ImVec2 rowMax = ImVec2(rowMin.x + ImGui::GetContentRegionAvail().x,
                    rowMin.y + ImGui::GetFrameHeightWithSpacing());
                ImGui::GetWindowDrawList()->AddRectFilled(
                    rowMin, rowMax, IM_COL32(60, 100, 180, 80));
            }

            // 可见性按钮
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f));

            if (ImGui::SmallButton(layer->IsVisible() ? " * " : " _ "))
            {
                layer->SetVisible(!layer->IsVisible());
                doc->GetScene().MarkDirty();
                //doc->GetEditor().GetGipEditor().MarkDirty(); // ← 加这行，刷新夹点
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(layer->IsVisible() ? "点击隐藏" : "点击显示");
            ImGui::SameLine();

            // 锁定按钮
            if (ImGui::SmallButton(layer->IsLocked() ? " L " : " U "))
                layer->SetLocked(!layer->IsLocked());
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(layer->IsLocked() ? "已锁定，点击解锁" : "点击锁定");

            ImGui::PopStyleColor(3);
            ImGui::SameLine();

            // 颜色选择器
            float col[4] = {
                layer->GetColor().x, layer->GetColor().y,
                layer->GetColor().z, layer->GetColor().w
            };
            if (ImGui::ColorEdit4("##col", col,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar))
            {
                layer->SetColor({ col[0], col[1], col[2], col[3] });
                doc->GetScene().MarkDirty();
				// 颜色修改后，可能需要刷新夹点等依赖颜色的 UI 元素
                /*printf("SetColor doc=%p LayerID=%u color=(%.2f,%.2f,%.2f)\n",
                    (void*)layer , layer->GetID(), col[0], col[1], col[2]);*/
            }
            ImGui::SameLine();

            // 图层名（单击选中，双击重命名）
            if (ImGui::Selectable(layer->GetName().c_str(), isActive,
                ImGuiSelectableFlags_AllowDoubleClick))
            {
                lm.SetActiveLayerID(id);
                lm.SetActiveLayerID(id);

                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    ImGui::OpenPopup("##RenameLayer");
            }

            // 右键菜单
            if (ImGui::BeginPopupContextItem("##LayerCtx"))
            {
                if (ImGui::MenuItem("设为当前层"))
                {
                    lm.SetActiveLayerID(id);
                    lm.SetActiveLayerID(id);
                }
                if (ImGui::MenuItem("重命名"))
                    ImGui::OpenPopup("##RenameLayer");
                ImGui::Separator();
                if (ImGui::MenuItem(layer->IsVisible() ? "隐藏图层" : "显示图层"))
                {
                    layer->SetVisible(!layer->IsVisible());
                    doc->GetScene().MarkDirty();
                }
                if (ImGui::MenuItem(layer->IsLocked() ? "解锁图层" : "锁定图层"))
                    layer->SetLocked(!layer->IsLocked());
                if (id != Layer::DefaultLayerID)
                {
                    ImGui::Separator();
                    if (ImGui::MenuItem("删除图层"))
                    {
                        lm.RemoveLayer(id);
                        if (lm.GetActiveLayerID() == id)
                        { 
                            lm.SetActiveLayerID(Layer::DefaultLayerID);
                        }
                        doc->GetScene().MarkDirty();
                        ImGui::EndPopup();
                        ImGui::PopID();
                        break;
                    }
                }
                ImGui::EndPopup();
            }

            // 重命名弹窗
            if (ImGui::BeginPopup("##RenameLayer"))
            {
                static char nameBuf[64] = {};
                if (ImGui::IsWindowAppearing())
                    strncpy_s(nameBuf, layer->GetName().c_str(), sizeof(nameBuf) - 1);

                ImGui::Text("重命名图层:");
                ImGui::SetNextItemWidth(220.f);
                bool confirm = ImGui::InputText("##RenameInput", nameBuf, sizeof(nameBuf),
                    ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::SameLine();
                confirm |= ImGui::Button("确认");
                if (confirm && strlen(nameBuf) > 0)
                {
                    layer->SetName(nameBuf);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }

        ImGui::EndChild();

        // ── 状态栏 + 移动选中到当前层 ───────────────────
        ImGui::Separator();
        if (auto* active = lm.GetLayer(lm.GetActiveLayerID()))
            ImGui::TextDisabled("当前层: %s   共 %d 层",
                active->GetName().c_str(), (int)ids.size());

        auto selected = editor.GetSelectedObjects();
        if (!selected.empty())
        {
            ImGui::SameLine();
            if (ImGui::Button("移动选中到此层"))
            {
                for (auto* obj : selected)
                {
                    if (obj->IsKindOf<Entity>())
                        static_cast<Entity*>(obj)->SetLayerId(lm.GetActiveLayerID());
                }
                doc->GetScene().MarkDirty();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("将选中的 %d 个对象移动到当前图层", (int)selected.size());
        }

        ImGui::End();
    }

    void UIManager::ShowAxisgridManager(DocumentManager& dm)
    {
        if (!m_showAxisGrid)
            return;

        Document* doc = dm.GetActive();
        if (!doc)
        {
            m_showAxisGrid = false;
            return;
        }
        EditorContext& editor = doc->GetEditor();

        ImGui::SetNextWindowPos(ImVec2(250, 250), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(250, 150), ImVec2(500, 400));
        std::string title = "轴网设置 - " + doc->GetName();
        if (ImGui::Begin(title.c_str(), &m_showAxisGrid,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking))
        {
            // 本地数据结构（保持在函数内的静态变量中以便跨帧保存）
            struct Axis { float pos; float realPos; bool vertical; bool visible; std::string name; };
            static std::vector<Axis> axes;
            static int axisCounter = 1;
            // 0 = 横向, 1 = 纵向
            static int addVertical = 1;
            static int dragging = -1;
            static float    axisExtend = 1.5f;   // 轴线出头（米）
            static XMFLOAT2 gridOrigin = { 0.0f, 0.0f }; // 轴网左下角交叉点世界坐标
            // ── 将 realPos 同步到归一化 pos ──────────────────────────────
            // 分别取纵轴/横轴的 realPos 范围，然后把 [min,max] 映射到 [0.05, 0.95]
            auto syncPos = [&]()
                {
                    for (int dir = 0; dir <= 1; ++dir)
                    {
                        bool isV = (dir == 1);
                        float mn = FLT_MAX, mx = -FLT_MAX;
                        for (auto& a : axes)
                            if (a.vertical == isV)
                            {
                                mn = std::min(mn, a.realPos); mx = std::max(mx, a.realPos);
                            }
                        if (mn == FLT_MAX) continue;
                        float range = mx - mn;
                        for (auto& a : axes)
                        {
                            if (a.vertical != isV) continue;
                            a.pos = (range > 1e-6f)
                                ? 0.05f + (a.realPos - mn) / range * 0.90f
                                : 0.5f;
                        }
                    }
                };

            // ── 将归一化 pos 反算回 realPos（拖拽后用）────────────────────
            auto syncRealFromPos = [&](int idx)
                {
                    bool isV = axes[idx].vertical;
                    float mn = FLT_MAX, mx = -FLT_MAX;
                    float pmin = FLT_MAX, pmax = -FLT_MAX;
                    for (auto& a : axes)
                        if (a.vertical == isV)
                        {
                            mn = std::min(mn, a.realPos); mx = std::max(mx, a.realPos);
                            pmin = std::min(pmin, a.pos);  pmax = std::max(pmax, a.pos);
                        }
                    float prange = pmax - pmin;
                    float rrange = mx - mn;
                    if (prange > 1e-6f && rrange > 1e-6f)
                        axes[idx].realPos = mn + (axes[idx].pos - pmin) / prange * rrange;
                };
            // ── 纵/横轴世界坐标范围 ─────────────────────────────────────
            auto calcRange = [&](float& vMin, float& vMax, float& hMin, float& hMax)
                {
                    vMin = FLT_MAX; vMax = -FLT_MAX;
                    hMin = FLT_MAX; hMax = -FLT_MAX;
                    for (auto& a : axes)
                    {
                        if (a.vertical) { vMin = std::min(vMin, a.realPos); vMax = std::max(vMax, a.realPos); }
                        else { hMin = std::min(hMin, a.realPos); hMax = std::max(hMax, a.realPos); }
                    }
                    if (vMin == FLT_MAX) { vMin = 0.0f; vMax = 10.0f; }
                    if (hMin == FLT_MAX) { hMin = 0.0f; hMax = 10.0f; }
                };
            // 顶部工具条：选择方向，添加轴
            ImGui::Text("方向："); ImGui::SameLine();
            ImGui::RadioButton("纵向", &addVertical, 1); ImGui::SameLine();
            ImGui::RadioButton("横向", &addVertical, 0);
            ImGui::SameLine();
            if (ImGui::Button("添加轴"))
            {
                Axis a;
                a.vertical = (addVertical != 0);
                a.visible = true;
                float maxR = 0.0f;
                for (auto& x : axes) if (x.vertical == a.vertical) maxR = std::max(maxR, x.realPos);
                a.realPos = axes.empty() ? 0.0f : maxR + 1.0f;
                a.pos = 0.5f;
                std::ostringstream ss;
                ss << "轴" << axisCounter++;
                a.name = ss.str();
                axes.push_back(std::move(a));
                syncPos();
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            ImGui::InputFloat("出头", &axisExtend, 0.5f, 1.0f, "%.1f");
            axisExtend = std::max(0.0f, axisExtend);

            // 原点坐标
            ImGui::Text("原点："); ImGui::SameLine();
            ImGui::SetNextItemWidth(80.0f);
            ImGui::InputFloat("X##ox", &gridOrigin.x, 1.0f, 10.0f, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80.0f);
            ImGui::InputFloat("Y##oy", &gridOrigin.y, 1.0f, 10.0f, "%.2f");

            ImGui::Columns(2, "axis_cols");


            // 两列布局：左画布，右表格
            ImGui::Columns(2, "axis_cols");

            // 左侧画布
            // 使画布宽度随窗口调整（使用 0 宽度填充列宽），高度设置为较小值以缩短黑框
            ImGui::BeginChild("AxisCanvas", ImVec2(0, 220), true);
            {
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 canvas_pos = ImGui::GetCursorScreenPos(); // top-left
                ImVec2 canvas_size = ImGui::GetContentRegionAvail();
                if (canvas_size.x < 20) canvas_size.x = 480;
                if (canvas_size.y < 20) canvas_size.y = 200;

                // background
                draw_list->AddRectFilled(canvas_pos,
                    ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                    IM_COL32(10, 10, 10, 255));
                // ── 画布布局 ──────────────────────────────────────────────
                // 留出 margin 给轴号圆泡
                const float MARGIN = 32.0f;   // 圆泡区域（像素）
                const float BUBBLE_R = 10.0f;  // 圆泡半径

                float gL = canvas_pos.x + MARGIN;     // grid left
                float gR = canvas_pos.x + canvas_size.x - MARGIN; // grid right
                float gT = canvas_pos.y + MARGIN;     // grid top
                float gB = canvas_pos.y + canvas_size.y - MARGIN; // grid bottom
                float gW = gR - gL;
                float gH = gB - gT;

                // 网格边框（淡色参考框）
                draw_list->AddRect(ImVec2(gL, gT), ImVec2(gR, gB), IM_COL32(50, 50, 50, 180), 0, 0, 1.0f);

                ImVec2 mouse_pos = ImGui::GetIO().MousePos;
                bool hovered = ImGui::IsItemHovered();

                // ── 辅助：按 pos 排好序的同向轴索引 ──────────────────────
                auto sortedByDir = [&](bool vertical) -> std::vector<int>
                    {
                        std::vector<int> idx;
                        for (int i = 0; i < (int)axes.size(); ++i)
                            if (axes[i].vertical == vertical && axes[i].visible)
                                idx.push_back(i);
                        std::sort(idx.begin(), idx.end(),
                            [&](int a, int b) { return axes[a].pos < axes[b].pos; });
                        return idx;
                    };
                ImU32 lineCol = IM_COL32(200, 200, 50, 255);
                ImU32 extCol = IM_COL32(200, 200, 50, 120); // 出头部分（淡）
                ImU32 bubbleBg = IM_COL32(30, 30, 30, 230);
                ImU32 bubbleFg = IM_COL32(220, 200, 60, 255);
                ImU32 dimCol = IM_COL32(100, 220, 255, 220);
                ImU32 tickCol = IM_COL32(100, 220, 255, 140);

                // ── 辅助：画轴号圆泡 ──────────────────────────────────
                auto drawBubble = [&](float cx, float cy, const std::string& name)
                    {
                        draw_list->AddCircleFilled(ImVec2(cx, cy), BUBBLE_R, bubbleBg);
                        draw_list->AddCircle(ImVec2(cx, cy), BUBBLE_R, bubbleFg, 0, 1.5f);
                        ImVec2 tsz = ImGui::CalcTextSize(name.c_str());
                        draw_list->AddText(ImVec2(cx - tsz.x * 0.5f, cy - tsz.y * 0.5f), bubbleFg, name.c_str());
                    };

                // ── 绘制轴线 + 出头 + 圆泡 ───────────────────────────
                for (size_t i = 0; i < axes.size(); ++i)
                {
                    if (!axes[i].visible) continue;

                    if (axes[i].vertical)
                    {
                        float x = gL + axes[i].pos * gW;

                        // 主体：网格范围内（实线）
                        draw_list->AddLine(ImVec2(x, gT), ImVec2(x, gB), lineCol, 1.5f);
                        // 出头：超出网格到 margin 区（淡线）
                        draw_list->AddLine(ImVec2(x, canvas_pos.y + MARGIN * 0.15f), ImVec2(x, gT), extCol, 1.5f);
                        draw_list->AddLine(ImVec2(x, gB), ImVec2(x, canvas_pos.y + canvas_size.y - MARGIN * 0.15f), extCol, 1.5f);
                        // 圆泡
                        drawBubble(x, canvas_pos.y + BUBBLE_R + 2.0f, axes[i].name);
                        drawBubble(x, canvas_pos.y + canvas_size.y - BUBBLE_R - 2.0f, axes[i].name);

                        // 拖拽命中检测
                        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                            if (std::abs(mouse_pos.x - x) < 6.0f && mouse_pos.y > gT && mouse_pos.y < gB)
                                dragging = (int)i;
                    }
                    else
                    {
                        float y = gT + axes[i].pos * gH;

                        draw_list->AddLine(ImVec2(gL, y), ImVec2(gR, y), lineCol, 1.5f);
                        draw_list->AddLine(ImVec2(canvas_pos.x + MARGIN * 0.15f, y), ImVec2(gL, y), extCol, 1.5f);
                        draw_list->AddLine(ImVec2(gR, y), ImVec2(canvas_pos.x + canvas_size.x - MARGIN * 0.15f, y), extCol, 1.5f);
                        drawBubble(canvas_pos.x + BUBBLE_R + 2.0f, y, axes[i].name);
                        drawBubble(canvas_pos.x + canvas_size.x - BUBBLE_R - 2.0f, y, axes[i].name);

                        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                            if (std::abs(mouse_pos.y - y) < 6.0f && mouse_pos.x > gL && mouse_pos.x < gR)
                                dragging = (int)i;
                    }
                }

                // ── 距离标注（相邻同向轴之间）──────────────────────────
                // 纵向：尺寸线画在网格顶部内侧
                {
                    auto idx = sortedByDir(true);
                    for (int k = 1; k < (int)idx.size(); ++k)
                    {
                        int  a = idx[k - 1], b = idx[k];
                        float dist = std::abs(axes[b].realPos - axes[a].realPos);
                        float xa = gL + axes[a].pos * gW;
                        float xb = gL + axes[b].pos * gW;
                        float xm = (xa + xb) * 0.5f;
                        float y0 = gT + 6.0f;

                        draw_list->AddLine(ImVec2(xa, y0), ImVec2(xa, y0 + 8.0f), tickCol, 1.0f);
                        draw_list->AddLine(ImVec2(xb, y0), ImVec2(xb, y0 + 8.0f), tickCol, 1.0f);
                        draw_list->AddLine(ImVec2(xa, y0 + 4.0f), ImVec2(xb, y0 + 4.0f), tickCol, 1.0f);

                        char buf[32]; snprintf(buf, sizeof(buf), "%.2f", dist);
                        ImVec2 tsz = ImGui::CalcTextSize(buf);
                        draw_list->AddRectFilled(
                            ImVec2(xm - tsz.x * 0.5f - 2, y0 + 10.0f),
                            ImVec2(xm + tsz.x * 0.5f + 2, y0 + 10.0f + tsz.y), IM_COL32(12, 12, 12, 200));
                        draw_list->AddText(ImVec2(xm - tsz.x * 0.5f, y0 + 10.0f), dimCol, buf);
                    }
                }

                // 横向：尺寸线画在网格左侧内侧
                {
                    auto idx = sortedByDir(false);
                    for (int k = 1; k < (int)idx.size(); ++k)
                    {
                        int  a = idx[k - 1], b = idx[k];
                        float dist = std::abs(axes[b].realPos - axes[a].realPos);
                        float ya = gT + axes[a].pos * gH;
                        float yb = gT + axes[b].pos * gH;
                        float ym = (ya + yb) * 0.5f;
                        float x0 = gL + 6.0f;

                        draw_list->AddLine(ImVec2(x0, ya), ImVec2(x0 + 8.0f, ya), tickCol, 1.0f);
                        draw_list->AddLine(ImVec2(x0, yb), ImVec2(x0 + 8.0f, yb), tickCol, 1.0f);
                        draw_list->AddLine(ImVec2(x0 + 4.0f, ya), ImVec2(x0 + 4.0f, yb), tickCol, 1.0f);

                        char buf[32]; snprintf(buf, sizeof(buf), "%.2f", dist);
                        ImVec2 tsz = ImGui::CalcTextSize(buf);
                        draw_list->AddRectFilled(
                            ImVec2(x0 + 12.0f, ym - tsz.y * 0.5f - 1),
                            ImVec2(x0 + 12.0f + tsz.x + 4, ym + tsz.y * 0.5f + 1), IM_COL32(12, 12, 12, 200));
                        draw_list->AddText(ImVec2(x0 + 14.0f, ym - tsz.y * 0.5f), dimCol, buf);
                    }
                }

                // ── 拖拽 ──────────────────────────────────────────────
                if (dragging >= 0 && dragging < (int)axes.size() &&
                    ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    if (axes[dragging].vertical)
                        axes[dragging].pos = std::clamp((mouse_pos.x - gL) / gW, 0.0f, 1.0f);
                    else
                        axes[dragging].pos = std::clamp((mouse_pos.y - gT) / gH, 0.0f, 1.0f);
                    syncRealFromPos(dragging);
                }
                if (dragging >= 0 && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                    dragging = -1;

                ImGui::EndChild();
            }

            ImGui::NextColumn();


            // 右侧轴表
            ImGui::BeginChild("AxisTable", ImVec2(0, 220), false);
            {
                // 5列：名称 | 方向 | 位置(m) | 显示 | 操作
                bool needSync = false;

                if (ImGui::BeginTable("atc", 4,
                    ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_BordersInnerV |
                    ImGuiTableFlags_ScrollY,
                    ImVec2(0, 0)))
                {
                    ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                    ImGui::TableSetupColumn("方向", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                    ImGui::TableSetupColumn("位置", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableHeadersRow();

                    for (int i = 0; i < (int)axes.size(); ++i)
                    {
                        ImGui::TableNextRow();
                        ImGui::PushID(i);

                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(axes[i].name.c_str());

                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(axes[i].vertical ? "纵" : "横");

                        ImGui::TableSetColumnIndex(2);
                        ImGui::PushItemWidth(-1);
                        if (ImGui::InputFloat("##p", &axes[i].realPos, 0.5f, 1.0f, "%.2f"))
                            needSync = true;
                        ImGui::PopItemWidth();

                        ImGui::TableSetColumnIndex(3);
                        if (ImGui::SmallButton("中")) { axes[i].realPos = 0.0f; needSync = true; }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("X"))
                        {
                            axes.erase(axes.begin() + i);
                            ImGui::PopID();
                            needSync = true;
                            break;
                        }

                        ImGui::PopID();
                    }

                    ImGui::EndTable();
                }

                // ── 距离一览表（同向相邻轴） ──────────────────────────────
                if (!axes.empty())
                {
                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "间距一览");

                    // 纵向
                    std::vector<int> vIdx, hIdx;
                    for (int i = 0; i < (int)axes.size(); ++i)
                        (axes[i].vertical ? vIdx : hIdx).push_back(i);
                    auto cmp = [&](int a, int b) { return axes[a].realPos < axes[b].realPos; };
                    std::sort(vIdx.begin(), vIdx.end(), cmp);
                    std::sort(hIdx.begin(), hIdx.end(), cmp);

                    if (vIdx.size() >= 2)
                    {
                        ImGui::TextDisabled("纵轴：");
                        for (int k = 1; k < (int)vIdx.size(); ++k)
                        {
                            float d = axes[vIdx[k]].realPos - axes[vIdx[k - 1]].realPos;
                            ImGui::Text("  %s → %s : %.2f",
                                axes[vIdx[k - 1]].name.c_str(),
                                axes[vIdx[k]].name.c_str(), d);
                        }
                    }
                    if (hIdx.size() >= 2)
                    {
                        ImGui::TextDisabled("横轴：");
                        for (int k = 1; k < (int)hIdx.size(); ++k)
                        {
                            float d = axes[hIdx[k]].realPos - axes[hIdx[k - 1]].realPos;
                            ImGui::Text("  %s → %s : %.2f",
                                axes[hIdx[k - 1]].name.c_str(),
                                axes[hIdx[k]].name.c_str(), d);
                        }
                    }
                }

                if (needSync) syncPos();
            }
            ImGui::EndChild();

            ImGui::Columns(1);

            // 底部确认/取消
            ImGui::Separator();
            // ── 确定 / 取消 ───────────────────────────────────────────
            if (ImGui::Button("确定"))
            {
                float vMin, vMax, hMin, hMax;
                calcRange(vMin, vMax, hMin, hMax);

                auto& scene =  dm.GetActive()->GetScene();   //doc.GetScene();
                for (auto& a : axes)
                {
                    if (!a.visible) continue;

                    // realPos 减去各方向最小值，使最左纵轴/最底横轴对齐到 gridOrigin
                    // 世界坐标 = gridOrigin + (realPos - min)
                    XMFLOAT3 start, end;
                    if (a.vertical)
                    {
                        float wx = gridOrigin.x + (a.realPos - vMin);
                        float wyMin = gridOrigin.y + 0.0f - axisExtend; // hMin - hMin = 0
                        float wyMax = gridOrigin.y + (hMax - hMin) + axisExtend;
                        start = { wx, wyMin, 0.0f };
                        end = { wx, wyMax, 0.0f };
                    }
                    else
                    {
                        float wy = gridOrigin.y + (a.realPos - hMin);
                        float wxMin = gridOrigin.x + 0.0f - axisExtend;
                        float wxMax = gridOrigin.x + (vMax - vMin) + axisExtend;
                        start = { wxMin, wy, 0.0f };
                        end = { wxMax, wy, 0.0f };
                    }

                    if (!MiniCAD::Line(start, end, true).IsValid()) continue;
                    auto line = std::make_unique<LineEntity>(scene.NextObjectID(), start, end);
                    auto doc =  dm.GetActive();
				    auto layerId =  doc->GetLayerManager().GetActiveLayerID();
					line->SetLayerId(layerId); // 默认图层
                    scene.AddEntity(std::move(line));
                }
                m_showAxisGrid = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("取消")) m_showAxisGrid = false;
        }
        ImGui::End();
        return;
    }
    
    
    void UIManager::DrawStatusBar(DocumentManager& dm)
    {
        if (!dm.GetActive())
            return;

        ImGuiStyle& s = ImGui::GetStyle();

        //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.f, 3.f));
        //ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(8.f, 2.f));
        //ImGui::PushStyleColor(ImGuiCol_ChildBg,          ImVec4(0.15f, 0.15f, 0.15f, 1.f));

        ImGui::BeginChild("##StatusBar",
                          ImVec2(0.f, kStatusBarHeight),
                          false,
                          ImGuiWindowFlags_NoScrollbar);

          
        // ── 当前工具 ─────────────────────────────────────────────
        const char* toolNames[] = {
            "选择", "直线", "圆", "矩形", "圆弧", "椭圆", "多段线", "样条曲线", "复制", "移动", "镜像", "旋转", "图层","一键轴网","撤销", "重做"
        };
        ImGui::TextDisabled("工具:");
        ImGui::SameLine();
        ImGui::TextUnformatted(toolNames[static_cast<int>(m_activeTool)]);

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // ── 鼠标坐标 ─────────────────────────────────────────────
        const auto& state = m_viewportInput;
        ImGui::TextDisabled("坐标:");
        ImGui::SameLine();
        if (state.Hovered)
            ImGui::Text("X: %.1f  Y: %.1f", state.MouseLocal.x, state.MouseLocal.y);
        else
            ImGui::TextDisabled("---");

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        // ── 正交与捕捉 ────────────────────────────────────────────
        // 捕捉状态
        auto& style = ImGui::GetStyle();
        // 使用 ImGui 语义颜色（自动适配明暗主题）
        const ImVec4 colorActive = style.Colors[ImGuiCol_Text];
        const ImVec4 colorInactive = style.Colors[ImGuiCol_TextDisabled];
        static bool s_openSnapSettings = false;  // 右键触发标记
        ImGui::BeginChild("status_snap", ImVec2(80, 0), false);
        {

            bool snapEnabled = dm.GetActive()->GetEditor().IsSnapEnabled();
            ImVec2 btnPos = ImGui::GetCursorPos();
            ImGui::TextColored(snapEnabled ? colorActive : colorInactive, "捕捉(F3): ");
            ImGui::SameLine();
            ImGui::TextColored(snapEnabled ? colorActive : colorInactive, snapEnabled ? "开 " : "关  ");
            ImGui::SetCursorPos(btnPos);
            ImGui::InvisibleButton("snap_toggle", ImVec2(80, ImGui::GetTextLineHeight()));
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                dm.GetActive()->GetEditor().ToggleSnap();
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            {
                s_openSnapSettings = true;  // 标记为需要打开弹窗
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("左键切换捕捉开关\n右键打开捕捉设置");
            }
        }
        ImGui::EndChild();

        // ── 捕捉设置弹窗（在 EndChild 之后打开，保证 ID 栈正确）─────
        if (s_openSnapSettings)
        {
            ImGui::OpenPopup("##SnapSettingsPopup");
            s_openSnapSettings = false;
        }
        if (ImGui::BeginPopup("##SnapSettingsPopup", ImGuiWindowFlags_NoMove))
        {
            ImGui::TextDisabled("捕捉设置");
            ImGui::Separator();

            auto& editor = dm.GetActive()->GetEditor();

            // ── 总开关 ────────────────────────────────────────
            bool snapEnabled = editor.IsSnapEnabled();
            if (ImGui::Checkbox("启用捕捉 (F3)", &snapEnabled))
                editor.ToggleSnap();

            ImGui::Separator();
            ImGui::TextDisabled("捕捉类型");

            // ── 各捕捉类型复选框 ──────────────────────────────
            // 通过 Editor 的 SnapMask 控制；如果没有对应 API 可按需删减
            // ── 捕捉类型位掩码定义（与 SnapEngine 保持一致）──
            enum SnapMaskBits : int
            {
                SnapMask_Endpoint = 1 << 0,
                SnapMask_Midpoint = 1 << 1,
                SnapMask_Center = 1 << 2,
                SnapMask_Intersection = 1 << 3,
                SnapMask_Perpendicular = 1 << 4,
                SnapMask_Tangent = 1 << 5,
            };

            auto snapMask = editor.GetSnapMask();

            struct SnapItem { const char* label; int bit; bool implemented;};
            static const SnapItem kSnapItems[] =
            {
                { "端点",   SnapMask_Endpoint,true},
                { "中点",   SnapMask_Midpoint,true},
                { "圆心",   SnapMask_Center, false},
                { "交点",   SnapMask_Intersection,true},
                { "垂足",   SnapMask_Perpendicular,false},
                { "切点",   SnapMask_Tangent,false},

            };

            for (auto& item : kSnapItems)
            {
                if (!item.implemented)
                {
                    // 置灰，不可点击
                    ImGui::BeginDisabled(true);
                    bool dummy = false;
                    ImGui::Checkbox(item.label, &dummy);
                    ImGui::SameLine();
                    ImGui::TextDisabled("(未实现)");
                    ImGui::EndDisabled();
                    continue;
                }
                bool checked = (snapMask & item.bit) != 0;
                if (ImGui::Checkbox(item.label, &checked))
                {
                    if (checked) snapMask |= item.bit;
                    else         snapMask &= ~item.bit;
                    editor.SetSnapMask(snapMask);
                }
            }

            ImGui::Separator();

            // ── 捕捉半径 ──────────────────────────────────────
            float snapRadius = editor.GetSnapRadius();
            ImGui::SetNextItemWidth(120.f);
            if (ImGui::SliderFloat("捕捉半径", &snapRadius, 1.f, 30.f, "%.1f px"))
                editor.SetSnapRadius(snapRadius);

            ImGui::Spacing();
            if (ImGui::Button("关闭", ImVec2(80, 0)))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }



        ImGui::SameLine();

        // 正交状态
        ImGui::BeginChild("status_ortho", ImVec2(80, 0), false);
        {
            auto* activeDoc = dm.GetActive();
            if (activeDoc)
            {
                bool orthoEnabled = dm.GetActive()->GetEditor().IsOrthoEnabled();

                ImVec2 btnPos = ImGui::GetCursorPos();
                ImGui::TextColored(orthoEnabled ? colorActive : colorInactive, "正交(F8): ");
                ImGui::SameLine();
                ImGui::TextColored(orthoEnabled ? colorActive : colorInactive, orthoEnabled ? "开 " : "关 ");
                ImGui::SetCursorPos(btnPos);
                ImGui::InvisibleButton("ortho_toggle", ImVec2(80, ImGui::GetTextLineHeight()));
                if (ImGui::IsItemClicked())
                {
                    dm.GetActive()->GetEditor().ToggleOrtho();
                }
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        // ── 当前文档 ─────────────────────────────────────────────
        Document* active = dm.GetActive();
        ImGui::TextDisabled("文档:");
        ImGui::SameLine();
        if (active)
        {
            ImGui::TextUnformatted(active->GetName().c_str());
            float cooldown = 2.0f;

            bool justSaved = (dm.m_lastSaveTime > 0) &&
            (ImGui::GetTime() - dm.m_lastSaveTime) < cooldown;

        if (justSaved)
        {
        float alpha = 1.0f - (float)((ImGui::GetTime() - dm.m_lastSaveTime) / cooldown);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.f, 0.4f, alpha));
        ImGui::TextUnformatted("● 已保存");
        ImGui::PopStyleColor();
        }
        else
        {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.8f, 0.2f, 1.f));
        ImGui::TextUnformatted("● 未保存");
        ImGui::PopStyleColor();
        }
        }
        else
        {
            ImGui::TextDisabled("无");
        }

        // ── 右侧：文档数量 ───────────────────────────────────────
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "共 %zu 个文档",
                     dm.GetAll().size());
            float tw = ImGui::CalcTextSize(buf).x;
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - tw - s.WindowPadding.x);
            ImGui::TextDisabled("%s", buf);
        }

        ImGui::EndChild();
        //ImGui::PopStyleColor();
        //ImGui::PopStyleVar(2);
    }

    void UIManager::InitToolIcons()
    { 
        const char* iconPaths[] = {
                "icons/Arc.png",
                "icons/Circle.png",
                "icons/Copy.png",
                "icons/Cursor.png",
                "icons/Ellipse.png",
                "icons/Line.png",
                "icons/Mirror.png",
                "icons/Move.png",
                "icons/Pline.png",
                "icons/Rect.png",
                "icons/Rotate.png",
                "icons/Spline.png",
                "icons/Axisgrid.png",
                "icons/LayerClose.png",
                "icons/Redo.png",
                "icons/Undo.png",
        }; 

        m_toolIcons.clear(); 

        for (auto path : iconPaths)
        {
            std::filesystem::path p(path);
            std::string key = p.stem().string();

            auto srv = LoadTextureFromFile(path);
            if (!srv)
                continue;

            m_toolIcons.emplace(key, std::move(srv));
        }   
    }
      
    ImTextureID  UIManager::LoadTextureFromFile(const char* path)
    {
        // 用 stb_image 读取图片
        int w, h, ch;
        unsigned char* pixels = stbi_load(path, &w, &h, &ch, 4); // 强制 RGBA
        if (!pixels) return NULL;

        // 创建 D3D11 Texture2D
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = w;
        desc.Height = h;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = pixels;
        initData.SysMemPitch = w * 4;

        ID3D11Texture2D* tex = nullptr;
        m_device->CreateTexture2D(&desc, &initData, &tex);
        stbi_image_free(pixels);

        if (!tex) return NULL;

        // 创建 SRV（Shader Resource View），这才是 ImGui 需要的
        ID3D11ShaderResourceView* srv = nullptr;
        HRESULT hr = m_device->CreateShaderResourceView(tex, nullptr, &srv);
        if (FAILED(hr) || !srv)
        {
            tex->Release();
            return NULL;
        }

        return (ImTextureID)srv;
    }

   

}  
