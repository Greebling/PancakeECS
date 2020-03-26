#pragma once

#include <vector>
#include "../libs/robin-map/include/tsl/robin_map.h"
#include "ComponentData.h"


constexpr IndexType BASE_ENTITY_VECTOR_SIZE = 16;

template<typename... Ts>
class ComponentView;

class ComponentVectorBase
{
public:
	ComponentVectorBase()
	{
		entityIndex = new tsl::robin_map<EntityID, IndexType>();
	}
	
	virtual ~ComponentVectorBase() = default;
	
	virtual void RemoveComponentFrom(EntityID id) = 0;
	
	
	/// \return a map containing all entities in the ComponentVector
	[[nodiscard]] const tsl::robin_map<EntityID, IndexType> &getEntities() const
	{
		return *entityIndex;
	}
	
	size_t Size()
	{
		return entityIndex->size();
	}

protected:
	/// Maps an EntityID to the index of the component it owns
	tsl::robin_map<EntityID, IndexType> *entityIndex;
};

template<typename ComponentType>
class ComponentVector : public ComponentVectorBase
{
public:
	ComponentVector()
			: ComponentVectorBase()
	{
		static_assert(std::is_base_of<ComponentData, ComponentType>::value, "Must derive from ComponentData!");
		
		_components = new std::vector<ComponentType>();
		_components->reserve(BASE_ENTITY_VECTOR_SIZE);
	}
	
	~ComponentVector() override
	{
		delete _components;
		delete entityIndex;
	}
	
	ECSManager* manager{nullptr};

private:
	template<typename ...>
	friend
	class ComponentView;
	
	std::vector<ComponentType> *_components;

public:
	ComponentType &AddComponent(EntityID id)
	{
		// should not add component if it is already there
		if (entityIndex->count(id))
			return (*_components)[(*entityIndex)[id]];
		
		entityIndex->insert(std::pair(id, _components->size()));
		
		_components->push_back(ComponentType());
		_components->back().id = id;
		_components->back().manager = manager;
		
		return _components->back();
	}
	
	void RemoveComponentFrom(EntityID id) override
	{
		RemoveComponent(id);
	}
	
	void RemoveComponent(EntityID id)
	{
		if (!entityIndex->count(id))
			return;
		
		(*_components)[(*entityIndex)[id]] = _components->back();
		(*entityIndex)[_components->back().id] = (*entityIndex)[id];
		entityIndex->erase(id);
		_components->pop_back();
	}
	
	ComponentType *GetComponent(EntityID id)
	{
		if (!Contains(id))
			return nullptr;
		
		return &(*_components)[(*entityIndex)[id]];
	}
	
	ComponentType &operator[](size_t index)
	{
		return (*_components)[index];
	}
	
	[[nodiscard]] bool Contains(EntityID id) const
	{
		return entityIndex->count(id) != 0;
	}
	
	[[nodiscard]] IndexType IndexOf(EntityID id) const
	{
		assert(Contains(id));
		
		return (*entityIndex)[id];
	}
};
