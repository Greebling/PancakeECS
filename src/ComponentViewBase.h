#pragma once


#include "ctpl_stl.h"
#include "TypeId.h"
#include "EntityID.h"

class ComponentViewBase
{
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
	/// The thread pool that is used for the Parallel_Foreach method
	inline static ctpl::thread_pool tPool = ctpl::thread_pool(static_cast<int>(std::thread::hardware_concurrency()));
};