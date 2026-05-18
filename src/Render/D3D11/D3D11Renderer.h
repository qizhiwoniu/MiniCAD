#pragma once
#include "pch.h" 
#include "Shader.h"
#include "Core/Math/Mat4.hpp"
#include "../IRenderer.h"
#include "../VertexTypes.hpp"
#include <span>
#include <wrl/client.h>
#include <d3d11.h>

namespace MiniCAD
{
   
	class D3D11Renderer : public IRenderer
    {
    public:
        D3D11Renderer(ID3D11Device* device, ID3D11DeviceContext* context);

        virtual void BeginFrame(IRenderTarget& target, const ViewportDesc& viewport) override;
        virtual void EndFrame() override;
        virtual void Submit(std::span<const Vertex_P3_C4> verts, const Math::Mat4& viewProj, PrimitiveType type, bool depth = true, bool blend = false) override;
        
        virtual void* GetNativeDevice() override;
 
        ID3D11Device* GetDevice() { return m_device; }
        void SetClearColor(float r, float g, float b, float a = 1.0f)override
        {
            m_clearColor[0] = r;
            m_clearColor[1] = g;
            m_clearColor[2] = b;
            m_clearColor[3] = a;
        }

    private:
        void Initialize();

    private:
        ID3D11Device*        m_device = nullptr;
        ID3D11DeviceContext* m_context = nullptr;

        ComPtr<ID3D11Buffer> m_vb;
        ComPtr<ID3D11Buffer> m_cb;

        int m_maxVertices = 65536;
        float m_clearColor[4] = { 0.1f, 0.1f, 0.15f, 1.0f }; // 默认深蓝灰
        // ===== states =====
        ComPtr<ID3D11DepthStencilState> m_depthEnabled;
        ComPtr<ID3D11DepthStencilState> m_depthDisabled;
        ComPtr<ID3D11DepthStencilState> m_depthReadOnly;  // 透明用

        ComPtr<ID3D11RasterizerState>   m_rsNoCull;
        ComPtr<ID3D11BlendState>        m_blendAlpha;

        LineShader m_lineShader;
    };
}