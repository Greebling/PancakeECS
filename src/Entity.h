# pragma once

#include "EntityID.h"

class Entity
{
public:
	EntityID id;
	
	[[nodiscard]] bool IsAlive() const
	{
		return id.IsAlive();
	}
};