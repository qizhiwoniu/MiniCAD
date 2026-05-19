#include "LayerManager.h"   
#include "Layer.h"
#include <utility>
namespace MiniCAD
{
    LayerManager::LayerManager()
    {
        auto defaultLayer = std::make_unique<Layer>(Layer::DefaultLayerID, "Default");
        defaultLayer->SetColor({ 0.8f, 0.8f, 0.8f, 1.0f }); // 白色，可改成你想要的默认色
        m_layers[Layer::DefaultLayerID] = std::move(defaultLayer);
    }

    LayerID LayerManager::AddLayer(const std::string& name)
    {
        LayerID id = m_nextID.fetch_add(1);
        //m_layers[id] = std::make_unique<Layer>(id, name);
        auto layer = std::make_unique<Layer>(id, name);

        // ★ 自动分配颜色，循环取不同颜色
        static const Math::Color4 kPalette[] = {
            { 1.0f, 0.3f, 0.3f, 1.0f }, // 红
            { 0.3f, 1.0f, 0.3f, 1.0f }, // 绿
            { 0.3f, 0.6f, 1.0f, 1.0f }, // 蓝
            { 1.0f, 0.8f, 0.2f, 1.0f }, // 黄
            { 0.8f, 0.3f, 1.0f, 1.0f }, // 紫
            { 0.3f, 1.0f, 1.0f, 1.0f }, // 青
            { 1.0f, 0.5f, 0.1f, 1.0f }, // 橙
        };
        constexpr int kPaletteSize = sizeof(kPalette) / sizeof(kPalette[0]);
        layer->SetColor(kPalette[id % kPaletteSize]);

        m_layers[id] = std::move(layer);
        return id;
    }

    LayerID LayerManager::AddLayer(std::unique_ptr<Layer> layer)
    {
        LayerID id = layer->GetID();

        if (id == Layer::DefaultLayerID || m_layers.count(id))
        {
            return Layer::DefaultLayerID; // ID 已存在，拒绝添加
        }

        m_layers[id] = std::move(layer);


        if (id >= m_nextID)               // 保持 m_nextID 大于现有所有 ID
        {
            m_nextID = id + 1;
        }

        return id;
    }

    bool LayerManager::RemoveLayer(LayerID id)
    {
        if (id == Layer::DefaultLayerID)
            return false;

        return m_layers.erase(id) > 0;
    }

    Layer* LayerManager::GetLayer(LayerID id)
    {
        auto it = m_layers.find(id);
        return it != m_layers.end() ? it->second.get() : nullptr;
    }

    const Layer* LayerManager::GetLayer(LayerID id) const
    {
        auto it = m_layers.find(id);
        return it != m_layers.end() ? it->second.get() : nullptr;
    }

    std::vector<LayerID> LayerManager::GetAllLayerIDs() const
    {
        std::vector<LayerID> ids;
        ids.reserve(m_layers.size());
        for (auto& kv : m_layers) ids.push_back(kv.first);
        return ids;
    }

    void LayerManager::SetActiveLayerID(LayerID id)
    {
        if (m_layers.count(id)) m_activeLayerID = id;
    }

}
