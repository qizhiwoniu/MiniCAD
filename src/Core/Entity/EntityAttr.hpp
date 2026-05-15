#pragma once   
#include "../Math/Color4.hpp"
#include <cstdint>

namespace MiniCAD
{
	using LayerID = uint32_t;

	enum class LineType : uint8_t
	{
		SOLID = 0,
		DASHED,
		DOTTED,
		DASH_DOT
	};


	class EntityAttr
	{
	public:
		Math::Color4 Color = Math::Color4::White();
		LayerID   LayerId = 0;
		LineType  LineType = LineType::SOLID;
		float     LineWidth = 1.0;
		bool      Visible = true;
	};


}