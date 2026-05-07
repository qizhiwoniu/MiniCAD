#pragma once
#include <string>

namespace YGsoftware
{
	class Scene;
	class ICommand
	{
	public:
		virtual ~ICommand() = default;
		virtual void        Execute(Scene& scene) = 0;
		virtual void        Undo(Scene& scene) = 0;
		virtual std::string GetName()const = 0;
	};
}