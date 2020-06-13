#pragma once

#include <vector>
#include <limits>
#include <cassert>
#include <functional>
#include "../libs/robin-map/include/tsl/robin_map.h"
#include "TypeId.h"
#include "ECSManager.h"
#include "ComponentViewBase.h"
#include "Scene.h"

enum class UpdateType
{
	Automatic, Manual
};

template<typename ...ComponentTypes>
class ComponentView : public ComponentViewBase
{
public:
	/// Register the system in the ECS Manager and gets all currently eligible entities
	explicit ComponentView(ECSManager &manager);
	
	/// Adds a ComponentView in the currently active scene
	ComponentView();
	
	/// Adds the id to the ComponentView if it owns all interesting components
	/// \param type The ComponentId of the components type that was added
	/// \param id The EntityID to which the component was added
	void OnComponentAdded(ComponentId type, EntityID id) override;
	
	/// Removes the id to the ComponentView if it was registered and the ComponentId was interesting for the system
	/// \param type The ComponentId of the components type that was added
	/// \param id The EntityID from which the component was removed
	void OnComponentRemoved(ComponentId type, EntityID id) override;
	
	/// Updates the ComponentView registered entities
	void Update();
	
	/// Applies func for every component in the view. Using Lambdas for func is recommended
	/// \param func lambdaFunction
	void Foreach(std::function<void(ComponentTypes &...)> func);
	
	/// Applies func for every component in the view using all possible threads. Note that only functions that
	/// only modify the current component are legal to use in parallel_foreach
	/// \param func
	void Parallel_foreach(std::function<void(ComponentTypes &...)> func, int minSize = 256);
	
	std::size_t Size();

protected:
	/// Checks if this ComponentView is interested in the given ComponentID meaning
	/// whether or not it uses this componentType
	/// \param type
	/// \return
	bool IsInterested(ComponentId type);
	
	
	/// Puts the parameters into func
	/// \tparam Is List of Integers deducted from seq
	/// \param func The function the parameters shall be applied
	/// \param ComponentVectors The componentVectors containing the ComponentTypes
	/// \param startIndex The index where the componentIndex list of the current entity begins
	/// \param seq  The integer sequence with length equal to sizeOf...(ComponentTypes)
	template<size_t... Is>
	constexpr inline void ApplyFunction(const std::function<void(ComponentTypes &...)> &func,
	                                    const std::tuple<ComponentVector<ComponentTypes> &...> &ComponentVectors,
	                                    IndexType startIndex,
	                                    const std::index_sequence<Is...> seq) const
	{
		func((std::get<Is>(ComponentVectors)[(*_vectoredEntities)[startIndex + Is]])...);
	}

protected:
	UpdateType _currUpdateMode{UpdateType::Automatic}; // TODO: Implement function to change the way views are updated
	
	ECSManager &_manager;
	
	/// The ComponentIds of the types the ComponentView is interested in
	std::vector<ComponentId> _operatingTypes;
	
	/// Maps EntityID to the indices of its components in the respective ComponentVectors
	std::unique_ptr<tsl::robin_map<EntityID, IndexType>> _registeredEntities{};
	
	// saves the indices of the components of an entity behind next to each other
	std::unique_ptr<std::vector<IndexType> > _vectoredEntities{};
	
	mutable std::condition_variable suspendCV{};
};


template<typename... ComponentTypes>
ComponentView<ComponentTypes...>::ComponentView()
		:
		ComponentView(Scene::ACTIVE_SCENE->manager)
{
	assert(Scene::ACTIVE_SCENE != nullptr && "No active scene was found!");
}

template<typename... ComponentTypes>
ComponentView<ComponentTypes...>::ComponentView(ECSManager &manager)
		:_manager(manager)
{
	_operatingTypes = {TypeId<ComponentTypes>::GetId()...};
	
	_vectoredEntities = std::make_unique<std::vector<IndexType>>();
	_vectoredEntities->reserve(16);
	
	_registeredEntities = std::make_unique<tsl::robin_map<EntityID, IndexType>>();
	
	// register in entity system with types
	_manager.RegisterComponentSystem(this, _operatingTypes);
	
	// fill registeredEntities
	Update();
}

template<typename... ComponentTypes>
void ComponentView<ComponentTypes...>::OnComponentAdded(ComponentId type, EntityID id)
{
	if (!IsInterested(type))
		return;
	
	// Check if all ComponentArrays of the interesting ComponentTypes have the Entity registered
	if (!((_manager.GetComponents<ComponentTypes>() &&
	       _manager.GetComponents<ComponentTypes>()->Contains(id)) && ...))
	{
		return;
	}
	
	// component should not already be registered in registeredEntities as it had a missing component
	if (_registeredEntities->count(id))
		return;
	
	
	_registeredEntities->insert(std::pair(id, _vectoredEntities->size() - 1));
	( _vectoredEntities->push_back(_manager.GetComponents<ComponentTypes>()->IndexOf(id)), ...);
}

template<typename... ComponentTypes>
void ComponentView<ComponentTypes...>::OnComponentRemoved(ComponentId type, EntityID id)
{
	if (!IsInterested(type))
		return;
	
	// check if the id is registered in the system
	if (!_registeredEntities->count(id))
		return;
	
	
	IndexType componentVectorIndex = (*_registeredEntities)[id];
	
	
	if (componentVectorIndex ==
	    _vectoredEntities->size() - (1 + sizeof...(ComponentTypes))) // special case when we remove the last thing
	{
		for (int j = 0; j < sizeof...(ComponentTypes); ++j)
		{
			_vectoredEntities->pop_back();
		}
	} else
	{
		// swap last and to be deleted position
		const std::size_t startIndex = _vectoredEntities->size() - (1 + sizeof...(ComponentTypes));
		const std::size_t endIndex = _vectoredEntities->size() - 1;
		for (std::size_t i = startIndex; i < endIndex; ++i)
		{
			(*_vectoredEntities)[i - sizeof...(ComponentTypes)] = (*_vectoredEntities)[i];
		}
		
		// remove last component indices
		for (int j = 0; j < sizeof...(ComponentTypes); ++j)
		{
			_vectoredEntities->pop_back();
		}
	}
	
	_registeredEntities->erase(id);
}

template<typename... ComponentTypes>
bool ComponentView<ComponentTypes...>::IsInterested(ComponentId type)
{
	for (ComponentId currType : _operatingTypes)
	{
		if (currType == type)
			return true;
	}
	return false;
}


template<typename... ComponentTypes>
void ComponentView<ComponentTypes...>::Foreach(const std::function<void(ComponentTypes &...)> func)
{
	
	// the componentVectors the iterate over
	std::tuple<ComponentVector<ComponentTypes> &...> compVectors{*_manager.GetComponents<ComponentTypes>()...};
	
	
	constexpr auto seq = std::make_index_sequence<sizeof...(ComponentTypes)>();
	for (std::size_t i = 0; i < _vectoredEntities->size() / sizeof...(ComponentTypes); i += sizeof...(ComponentTypes))
	{
		ApplyFunction(func, compVectors, i, seq);
	}
}

template<typename... ComponentTypes>
void
ComponentView<ComponentTypes...>::Parallel_foreach(std::function<void(ComponentTypes &...)> func, const int minSize)
{
	// check if it actually makes sense to use parallel execution, depending on the input size
	size_t vectorSize = _vectoredEntities->size() / sizeof...(ComponentTypes);
	
	if (vectorSize < minSize)
	{
		// use single core implementation
		Foreach(func);
		return;
	}
	
	// use multi threaded implementation
	int nThreads = static_cast<int>(std::thread::hardware_concurrency());
	std::atomic_int nDone = 0;
	
	std::mutex mutex;
	std::unique_lock<std::mutex> lock(mutex);
	
	
	std::tuple<ComponentVector<ComponentTypes> &...> compVectors{*_manager.GetComponents<ComponentTypes>()...};
	for (int j = 0; j < nThreads; ++j)
	{
		std::size_t start = j * (vectorSize / nThreads);
		std::size_t end = std::min((j + 1) * (vectorSize / nThreads), vectorSize);
		
		tPool.push([&](int id)
		           {
			           constexpr auto seq = std::make_index_sequence<sizeof...(ComponentTypes)>();
			           for (std::size_t i = start; i < end; ++i)
			           {
				           ApplyFunction(func, compVectors, i, seq);
			           }
			
			
			           nDone++;
			           suspendCV.notify_one();
		           });
	}
	
	suspendCV.wait(lock, [&]() { return nDone == nThreads; });
}


template<typename... ComponentTypes>
size_t ComponentView<ComponentTypes...>::Size()
{
	return _registeredEntities->size();
}

template<typename... ComponentTypes>
void ComponentView<ComponentTypes...>::Update()
{
	// init data structures
	_vectoredEntities->clear();
	_vectoredEntities->reserve(16);
	
	_registeredEntities->clear();
	
	
	// look for already registered components in the system
	if ((_manager.GetComponents<ComponentTypes>() && ...))
	{
		std::vector<ComponentVectorBase *> interestingComponents = std::vector<ComponentVectorBase *>();
		interestingComponents.reserve(_operatingTypes.size());
		
		
		for (ComponentId currId : _operatingTypes)
		{
			interestingComponents.push_back(_manager.GetComponentsBase(currId));
		}
		
		// check if the components ids of the first componentVector are also registered in the other componentVectors
		// if yes, add them to the registered entities
		ComponentVectorBase *startComponents = interestingComponents[0];
		
		// search for smallest component vector to minimize work
		for (ComponentVectorBase *currComponents : interestingComponents)
		{
			if (currComponents->Size() < startComponents->Size())
			{
				startComponents = currComponents;
			}
		}
		
		for (const auto &currPair : startComponents->getEntities())
		{
			std::vector<IndexType> componentIndices = std::vector<IndexType>();
			componentIndices.reserve(_operatingTypes.size());
			
			// look for components belonging to the id in currPair in the other componentVectors
			bool isRegisteredEverywhere = true;
			for (ComponentVectorBase *currComponents : interestingComponents)
			{
				if (!currComponents->getEntities().count(currPair.first))
				{
					isRegisteredEverywhere = false;
					break;
				}
				
				// add components index to componentIndices
				componentIndices.push_back(currComponents->getEntities().at(currPair.first));
			}
			
			if (!isRegisteredEverywhere)
				continue;
			
			// register this id with componentIndices
			for (IndexType currIndex : componentIndices)
			{
				_vectoredEntities->push_back(currIndex);
			}
			
			_registeredEntities->insert(std::pair(currPair.first, _vectoredEntities->size() - 1));
		}
	}
}