#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <windows.h>
#include <d3d11.h>


namespace YGsoftware
{
    enum class UILayer
    {
        Background = 0,
        Main,
        Overlay
    };
    struct Vec2
    {
        float x = 0;
        float y = 0;
    };
    class UIElement
    {
    public:
        UIElement() = default;
        virtual ~UIElement() = default;

        // =========================
        // 基础属性
        // =========================
        RECT GetRect() const { return m_rect; }
        void SetRect(const RECT& rc) { m_rect = rc; }

        int GetX() const { return m_rect.left; }
        int GetY() const { return m_rect.top; }
        int GetWidth() const { return m_rect.right - m_rect.left; }
        int GetHeight() const { return m_rect.bottom - m_rect.top; }

        void SetPosition(int x, int y)
        {
            int w = GetWidth();
            int h = GetHeight();
            m_rect = { x, y, x + w, y + h };
        }

        void SetSize(int w, int h)
        {
            m_rect.right = m_rect.left + w;
            m_rect.bottom = m_rect.top + h;
        }

        // =========================
        // 层级 / 可见性
        // =========================
        void SetVisible(bool v) { m_visible = v; }
        bool IsVisible() const { return m_visible; }

        void SetLayer(UILayer layer) { m_layer = layer; }
        UILayer GetLayer() const { return m_layer; }

        void SetEnabled(bool e) { m_enabled = e; }
        bool IsEnabled() const { return m_enabled; }

        // =========================
        // 子元素管理
        // =========================
        void AddChild(std::unique_ptr<UIElement> child)
        {
            child->m_parent = this;
            m_children.push_back(std::move(child));
        }

        const std::vector<std::unique_ptr<UIElement>>& GetChildren() const
        {
            return m_children;
        }

        // =========================
        // 生命周期
        // =========================
        virtual void Update(float dt)
        {
            for (auto& c : m_children)
                c->Update(dt);
        }

        virtual void Draw(ID3D11DeviceContext* ctx)
        {
            if (!m_visible) return;

            for (auto& c : m_children)
                c->Draw(ctx);
        }

        // =========================
        // 输入系统
        // =========================
        virtual void OnMouseMove(int x, int y)
        {
            m_hovered = HitTest(x, y);

            for (auto& c : m_children)
                c->OnMouseMove(x, y);
        }

        virtual void OnMouseDown(int x, int y)
        {
            if (!m_enabled) return;

            if (HitTest(x, y))
            {
                m_pressed = true;
                m_focused = true;
            }

            // 从上往下分发（反向）
            for (auto it = m_children.rbegin(); it != m_children.rend(); ++it)
            {
                (*it)->OnMouseDown(x, y);
            }
        }

        virtual void OnMouseUp(int x, int y)
        {
            if (!m_enabled) return;

            if (m_pressed && HitTest(x, y))
            {
                if (m_onClick)
                    m_onClick();
            }

            m_pressed = false;

            for (auto& c : m_children)
                c->OnMouseUp(x, y);
        }

        // =========================
        // 命中检测
        // =========================
        virtual bool HitTest(int x, int y) const
        {
            return m_visible &&
                x >= m_rect.left && x <= m_rect.right &&
                y >= m_rect.top && y <= m_rect.bottom;
        }

        // =========================
        // 状态
        // =========================
        bool IsHovered() const { return m_hovered; }
        bool IsPressed() const { return m_pressed; }
        bool IsFocused() const { return m_focused; }

        // =========================
        // 事件绑定
        // =========================
        void SetOnClick(std::function<void()> cb)
        {
            m_onClick = cb;
        }

        void SetPosition(float x, float y) { m_position = { x, y }; }
        void SetSize(float w, float h) { m_size = { w, h }; }

        // 👉 获取
        Vec2 GetPosition() const { return m_position; }
        Vec2 GetSize() const { return m_size; }

    protected:
        Vec2 m_position;
        Vec2 m_size;
        RECT m_rect{ 0, 0, 0, 0 };

        bool m_visible = true;
        bool m_enabled = true;

        bool m_hovered = false;
        bool m_pressed = false;
        bool m_focused = false;

        UILayer m_layer = UILayer::Main;

        UIElement* m_parent = nullptr;
        std::vector<std::unique_ptr<UIElement>> m_children;

        std::function<void()> m_onClick;
    };
}