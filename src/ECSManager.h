#pragma once

#include <queue>
#include <cassert>
#include <tsl/robin_map.h>
#include "ComponentData.h"
#include "EntityID.h"
#include "ComponentVector.h"
#include "TypeId.h"
#include "ComponentViewBase.h"

class Entity
{
public:
	EntityID id;
	
	[[nodiscard]] bool IsAlive() const;
};

////////////////////////////////////////////////////////

class ECSManager;

/// A wrapper for a pointer to a component living in a ComponentVector
/// \tparam ComponentType The type of the component it is pointing to
template<typename ComponentType>
struct ComponentPointer
{
	ComponentPointer();
	
	ComponentPointer(EntityID id, ECSManager &manager);
	
	explicit ComponentPointer(ECSManager &manager);
	
	ComponentPointer(const ComponentPointer &pointer);
	
	EntityID id;
	
	bool IsValid() const;
	
	/// Not recommended to use this as a raw pointer to a component might be i
	/// invalidated by adding new components of the same type
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

private:
	template<typename>
	friend
	struct ComponentPointer;
	
	template<typename ...>
	friend
	class ComponentView;
	
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
	ComponentPointer<ComponentType> AddComponent(EntityID id);
	
	/// Returns a ComponentPointer to the Component that is owned by the id.
	/// \tparam ComponentType
	/// \param id
	/// \return
	template<typename ComponentType>
	ComponentPointer<ComponentType> GetComponent(EntityID id);
	
	ComponentVectorBase *GetComponentsBase(ComponentId componentType);
	
	template<typename ComponentType>
	void RemoveComponent(EntityID id);

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
	
	/// Returns the given component of the Entity
	/// \tparam ComponentType Deriving from ComponentData
	/// \return A pointer to the component or nullptr if the GameActor did not have the ComponentType
	template<typename ComponentType>
	ComponentType *GetComponentDirect(EntityID id);
	
	void NotifyOnAdd(ComponentId componentType, EntityID id);
	
	void NotifyOnRemove(ComponentId componentType, EntityID id);
};


////////////////////////////////////////////////////////

template<typename ComponentType>
ComponentPointer<ComponentType>::ComponentPointer()
		:id()
		 , _manager(nullptr)
{

}

template<typename ComponentType>
ComponentPointer<ComponentType>::ComponentPointer(EntityID id, ECSManager &manager)
		: id(id)
		  , _manager(&manager)
{
	static_assert(std::is_base_of<ComponentData, ComponentType>::value,
	              "ComponentType must derive from ComponentData!");
}

template<typename ComponentType>
ComponentPointer<ComponentType>::ComponentPointer(ECSManager &manager)
		: id(0, 0)
		  , _manager(&manager)
{
}

template<typename ComponentType>
ComponentPointer<ComponentType>::ComponentPointer(const ComponentPointer &pointer)
		:id(pointer.id)
		 , _manager(pointer._manager)
{
}


template<typename ComponentType>
ComponentType *ComponentPointer<ComponentType>::RawPointer()
{
	assert(_manager && "Manager must be set!");
	
	ComponentType *componentP = _manager->GetComponentDirect<ComponentType>(id);
	assert(componentP != nullptr && "ComponentPointer is invalid");
	return componentP;
}


template<typename ComponentType>
ComponentType &ComponentPointer<ComponentType>::operator*()
{
	assert(_manager && "Manager must be set!");
	
	ComponentType *componentP = _manager->GetComponentDirect<ComponentType>(id);
	assert(componentP != nullptr && "ComponentPointer is invalid");
	return *componentP;
}

template<typename ComponentType>
const ComponentType &ComponentPointer<ComponentType>::operator*() const
{
	assert(_manager && "Manager must be set!");
	
	ComponentType *componentP = _manager->GetComponentDirect<ComponentType>(id);
	assert(componentP != nullptr && "ComponentPointer is invalid");
	return *componentP;
}

template<typename ComponentType>
ComponentType *ComponentPointer<ComponentType>::operator->()
{
	assert(_manager && "Manager must be set!");
	
	ComponentType *componentP = _manager->GetComponentDirect<ComponentType>(id);
	assert(id.IsAlive() && componentP && " ComponentPointer is invalid");
	
	return componentP;
}

template<typename ComponentType>
const ComponentType *ComponentPointer<ComponentType>::operator->() const
{
	assert(_manager && "Manager must be set!");
	
	ComponentType *componentP = _manager->GetComponentDirect<ComponentType>(id);
	assert(id.IsAlive() && componentP != nullptr && "ComponentPointer is invalid");
	
	return componentP;
}

template<typename ComponentType>
bool ComponentPointer<ComponentType>::IsValid() const
{
	return id.IsAlive() && (_manager->GetComponentDirect<ComponentType>(id));
}

////////////////////////////////////////////////////////

template<typename ComponentType>
ComponentPointer<ComponentType> ECSManager::AddComponent(EntityID id)
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
			return ComponentPointer<ComponentType>(id, *this);
		
		pComponents->AddComponent(id);
		
		NotifyOnAdd(componentTypeId, id);
		return ComponentPointer<ComponentType>(id, *this);
	}
	
	// else we need to add the type to componentVectors
	auto *createdComponents = new ComponentVector<ComponentType>();
	_componentVectors.insert(std::pair(componentTypeId, static_cast<ComponentVectorBase *>(createdComponents)));
	
	createdComponents->AddComponent(id);
	
	NotifyOnAdd(componentTypeId, id);
	return ComponentPointer<ComponentType>(id, *this);
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
ComponentPointer<ComponentType> ECSManager::GetComponent(EntityID id)
{
	return ComponentPointer<ComponentType>(id, *this);
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