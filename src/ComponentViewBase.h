// Created on 31.12.2019.

#pragma once


#include "ctpl_stl.h"
#include "TypeId.h"
#include "EntityID.h"

class ComponentViewBase {
public:
    /// Executed when a component with the ComponentID type for the EntityID id is added.
    /// Registers the given component in the system, if the id owns all the necessary components for the system.
    /// \param type
    /// \param id
    virtual void OnComponentAdded(ComponentId type, EntityID id) = 0;

    /// Executed when a component with the ComponentID type for the EntityID id is removed.
    /// Unregisters the given component from the system if it was registered.
    /// \param type
    /// \param id
    virtual void OnComponentRemoved(ComponentId type, EntityID id) = 0;

protected:
    static ctpl::thread_pool tPool;
};