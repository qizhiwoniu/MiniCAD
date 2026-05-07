#pragma once
#include "source/App/CommandStack/ICommand.h"
#include "source/Core/Object/Object.hpp"
#include "source/Core/Entity/PointEntity.hpp"
#include <DirectXMath.h>

namespace YGsoftware
{
    class DragPointCommand : public ICommand
    {
    public:
        DragPointCommand(Object::ObjectID id, const DirectX::XMFLOAT3& before, const DirectX::XMFLOAT3& after)
            : m_id(id), m_before(before), m_after(after) {}

        void Execute(Scene& scene) override
        {
            Apply(scene, m_after);
        }

        void Undo(Scene& scene) override
        {
            Apply(scene, m_before);
        }

        std::string GetName() const override { return "拖动点"; }

    private:
        void Apply(Scene& scene, const DirectX::XMFLOAT3& p) const
        {
            auto* obj = scene.GetEntity(m_id);
            if (!obj) return;
            if (obj->IsKindOf<PointEntity>())
            {
                auto* pt = static_cast<PointEntity*>(obj);
                pt->SetPosition(p);
            }
        }

        Object::ObjectID m_id;
        DirectX::XMFLOAT3 m_before;
        DirectX::XMFLOAT3 m_after;
    };
}
