#pragma once

#include <queue>
#include <cassert>
#include "../libs/robin-map/include/tsl/robin_map.h"

#include "ComponentData.h"
#include "EntityID.h"
#include "ComponentVector.h"
#include "TypeId.h"
#include "ComponentViewBase.h"
#include "Entity.h"


class ECSManager;

/// A wrapper for a pointer to a component living in a ComponentVector
/// \tparam ComponentType The type of the component it is pointing to
template<typename ComponentType>
struct ComponentHandle
{
	ComponentHandle();
	
	ComponentHandle(EntityID id, ECSManager &manager);
	
	explicit ComponentHandle(ECSManager &manager);
	
	explicit ComponentHandle(const ComponentType &component);
	
	ComponentHandle(const ComponentHandle &pointer);
	
	EntityID id;
	
	[[nodiscard]] bool IsValid() const;
	
	/// It is not recommended to use this as a raw pointer to a component as it might be
	/// invalidated by adding new components of the same type to the ecs manager
	/// \return A pointer to the actual memory location of the component
	ComponentType *RawPointer();
	
	ComponentType &operator*();
	
	const ComponentType &operator*() const;
	
	ComponentType *operator->();
	
	const ComponentType *operator->() const;

private:
	ECSManager *_manager;
};

////////////////////////////////////////////////////////

class ECSManager
{
public:
	ECSManager()
	{
		_deletedIndices = new std::deque<IndexType>();
		
		_entities = new std::vector<Entity>();
	}
	
	~ECSManager();

public:
	Entity *GetEntity(EntityID id);
	
	/// Creates a new Entity and returns its ID
	/// \return
	EntityID AddEntity();
	
	/// Removes a component from the ECS Manager and unregisters it from all ComponentSystems.
	/// \param id Owner of the Component
	/// \return Whether or not the component was successfully removed
	bool DestroyEntity(EntityID id);
	
	/// Ads the given ComponentType that belongs to the given id
	/// \tparam ComponentType Deriving from ComponentData
	/// \param id Owner of the Component
	/// \return
	template<typename ComponentType>
	ComponentHandle<ComponentType> AddComponent(EntityID id);
	
	/// Returns a ComponentHandle to the Component that is owned by the id.
	/// \tparam ComponentType
	/// \param id
	/// \return
	template<typename ComponentType>
	ComponentHandle<ComponentType> GetComponent(EntityID id);
	
	template<typename ComponentType>
	void RemoveComponent(EntityID id);

private:
	/// The list of entities the system might hold
	std::vector<Entity> *_entities;
	
	/// list of indices where entities were deleted
	std::deque<IndexType> *_deletedIndices;
	
	/// Maps the componentVectors to a given ComponentId that they are holding as a type
	tsl::robin_map<ComponentId, ComponentVectorBase *> _componentVectors;
	
	/// Maps the componentSystems to a given ComponentId that they are interested in
	tsl::robin_map<ComponentId, std::vector<ComponentViewBase *> > _componentSystems;
	
	/// place where the last insert of a new entity happened (if no index of _deletedIndices was used)
	IndexType _lastInsert{1};
	
	template<typename>
	friend
	struct ComponentHandle;
	
	template<typename ...>
	friend
	class ComponentView;

private:
	
	/// Adds a given system to the component system interested in the given types
	/// \param system
	/// \param componentId
	void RegisterComponentSystem(ComponentViewBase *system, const std::vector<ComponentId> &componentIds);
	
	/// Finds the vector of the given type
	/// \tparam ComponentType Deriving from ComponentData
	/// \return A vector of the given type or nullptr if there was not any
	template<typename ComponentType>
	ComponentVector<ComponentType> *GetComponents();
	
	/// Similar to GetComponents, but returns only the ComponentVectorBase
	/// \param componentType
	/// \return
	ComponentVectorBase *GetComponentsBase(ComponentId componentType);
	
	/// Returns the given component of the Entity
	/// \tparam ComponentType Deriving from ComponentData
	/// \return A pointer to the component or nullptr if the GameActor did not have the ComponentType
	template<typename ComponentType>
	ComponentType *GetComponentDirect(EntityID id);
	
	/// Updates all ComponentViews active about newly added components
	/// \param componentType
	/// \param id
	void NotifyOnAdd(ComponentId componentType, EntityID id);
	
	///  Updates all ComponentViews active about newly removed components
	/// \param componentType
	/// \param id
	void NotifyOnRemove(ComponentId componentType, EntityID id);
};


////////////////////////////////////////////////////////
// ComponentHandle implementations
////////////////////////////////////////////////////////

template<typename ComponentType>
ComponentHandle<ComponentType>::ComponentHandle()
		:id()
		 , _manager(nullptr)
{

}

template<typename ComponentType>
ComponentHandle<ComponentType>::ComponentHandle(EntityID id, ECSManager &manager)
		: id(id)
		  , _manager(&manager)
{
	static_assert(std::is_base_of<ComponentData, ComponentType>::value,
	              "ComponentType must derive from ComponentData!");
}

template<typename ComponentType>
ComponentHandle<ComponentType>::ComponentHandle(ECSManager &manager)
		: id(0, 0)
		  , _manager(&manager)
{
}

template<typename ComponentType>
ComponentHandle<ComponentType>::ComponentHandle(const ComponentType &component)
		:id(component.id)
		 , _manager(*component.manager)
{
}

template<typename ComponentType>
ComponentHandle<ComponentType>::ComponentHandle(const ComponentHandle &pointer)
		:id(pointer.id)
		 , _manager(pointer._manager)
{
}

template<typename ComponentType>
ComponentType *ComponentHandle<ComponentType>::RawPointer()
{
	assert(_manager && "Manager must be set!");
	
	ComponentType *componentP = _manager->GetComponentDirect<ComponentType>(id);
	assert(componentP != nullptr && "ComponentHandle is invalid");
	return componentP;
}


template<typename ComponentType>
ComponentType &ComponentHandle<ComponentType>::operator*()
{
	assert(_manager && "Manager must be set!");
	
	ComponentType *componentP = _manager->GetComponentDirect<ComponentType>(id);
	assert(componentP != nullptr && "ComponentHandle is invalid");
	return *componentP;
}

template<typename ComponentType>
const ComponentType &ComponentHandle<ComponentType>::operator*() const
{
	assert(_manager && "Manager must be set!");
	
	ComponentType *componentP = _manager->GetComponentDirect<ComponentType>(id);
	assert(componentP != nullptr && "ComponentHandle is invalid");
	return *componentP;
}

template<typename ComponentType>
ComponentType *ComponentHandle<ComponentType>::operator->()
{
	assert(_manager && "Manager must be set!");
	
	ComponentType *componentP = _manager->GetComponentDirect<ComponentType>(id);
	assert(id.IsAlive() && componentP && " ComponentHandle is invalid");
	
	return componentP;
}

template<typename ComponentType>
const ComponentType *ComponentHandle<ComponentType>::operator->() const
{
	assert(_manager && "Manager must be set!");
	
	ComponentType *componentP = _manager->GetComponentDirect<ComponentType>(id);
	assert(id.IsAlive() && componentP != nullptr && "ComponentHandle is invalid");
	
	return componentP;
}

template<typename ComponentType>
bool ComponentHandle<ComponentType>::IsValid() const
{
	return id.IsAlive() && (_manager->GetComponentDirect<ComponentType>(id));
}

////////////////////////////////////////////////////////
// ECSManager implementations
////////////////////////////////////////////////////////

template<typename ComponentType>
ComponentHandle<ComponentType> ECSManager::AddComponent(EntityID id)
{
	static_assert(std::is_base_of_v<ComponentData, ComponentType>,
	              "ComponentType has to derive from ComponentData!");
	
	auto componentTypeId = TypeId<ComponentType>::GetId();
	
	// check if we have an already existing ComponentVector for the type
	if (_componentVectors.count(componentTypeId))
	{
		auto *pComponents = static_cast<ComponentVector<ComponentType> *>(_componentVectors[componentTypeId]);
		
		// check if a component belonging to the id already exists in the ComponentVector
		if (pComponents->Contains(id))
			return ComponentHandle<ComponentType>(id, *this);
		
		pComponents->AddComponent(id);
		
		NotifyOnAdd(componentTypeId, id);
		return ComponentHandle<ComponentType>(id, *this);
	} else
	{
		// else we need to add the type to componentVectors
		auto *createdComponents = new ComponentVector<ComponentType>();
		createdComponents->manager = this;
		_componentVectors.insert(std::pair(componentTypeId, static_cast<ComponentVectorBase *>(createdComponents)));
		
		createdComponents->AddComponent(id);
		
		NotifyOnAdd(componentTypeId, id);
		return ComponentHandle<ComponentType>(id, *this);
	}
}

template<typename ComponentType>
ComponentType *ECSManager::GetComponentDirect(EntityID id)
{
	static_assert(std::is_base_of_v<ComponentData, ComponentType>,
	              "ComponentType has to derive from ComponentData!");
	
	auto componentTypeId = TypeId<ComponentType>::GetId();
	
	if (!_componentVectors.count(componentTypeId))
		return nullptr;
	
	auto *componentVector = static_cast<ComponentVector<ComponentType> *>(_componentVectors[componentTypeId]);
	
	ComponentType *component = componentVector->GetComponent(id);
	
	if (!component || !component->IsAlive())
		return nullptr;
	
	return component;
}

template<typename ComponentType>
ComponentHandle<ComponentType> ECSManager::GetComponent(EntityID id)
{
	return ComponentHandle<ComponentType>(id, *this);
}

template<typename ComponentType>
ComponentVector<ComponentType> *ECSManager::GetComponents()
{
	static_assert(std::is_base_of_v<ComponentData, ComponentType>,
	              "ComponentType has to derive from ComponentData!");
	
	auto componentTypeId = TypeId<ComponentType>::GetId();
	
	
	if (!_componentVectors.count(componentTypeId))
		return nullptr;
	
	auto *componentVector = static_cast<ComponentVector<ComponentType> *>(_componentVectors.at(componentTypeId));
	return componentVector;
}


template<typename ComponentType>
void ECSManager::RemoveComponent(EntityID id)
{
	static_assert(std::is_base_of_v<ComponentData, ComponentType>,
	              "ComponentType has to derive from ComponentData!");
	
	auto componentTypeId = TypeId<ComponentType>::GetId();
	
	// check if we have an already existing ComponentVector for the type
	if (!_componentVectors.count(componentTypeId))
		return;
	
	auto *pComponents = static_cast<ComponentVector<ComponentType> *>(_componentVectors.at(componentTypeId));
	
	pComponents->RemoveComponent(id);
	
	NotifyOnRemove(componentTypeId, id);
}