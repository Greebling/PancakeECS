#pragma once

#include "EntityID.h"

struct ComponentData {
public:
    EntityID id;

    bool IsAlive() const {
        return id.IsAlive();
    }
};
