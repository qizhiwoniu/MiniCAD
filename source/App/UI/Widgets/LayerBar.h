#pragma once
#include <string>
#include "ImGuiWidgetBase.h" 

namespace YGsoftware
{
    extern bool g_show_LayerBarPanel;
    class LayerBar : public ImGuiWidgetBase
    {
    public:
        virtual void        OnRender(Document& document) override;
        virtual const char* GetName() const  override;
    };
}