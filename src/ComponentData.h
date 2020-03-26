#pragma once

#include "EntityID.h"

class ECSManager;

struct ComponentData
{
public:
	ComponentData() = default;
	
	EntityID id;
	ECSManager *manager{nullptr};
	
	[[nodiscard]] bool IsAlive() const
	{
		return id.IsAlive();
	}
};
