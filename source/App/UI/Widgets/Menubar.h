#pragma once
#include <string>
#include "ImGuiWidgetBase.h" 
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_internal.h>

namespace YGsoftware
{
    class Menubar : public ImGuiWidgetBase
    {
    public:
        virtual void        OnRender(Document& editor) override;
        virtual const char* GetName() const  override;


        static float        GetHeight() { return ImGui::GetFrameHeight(); }
        static float        s_bottomY;  // 菜单栏实际底部 Y
    };
}