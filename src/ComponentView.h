#pragma once

#include <vector>
#include <limits>
#include <cassert>
#include <functional>
#include <tsl/robin_map.h>
#include "TypeId.h"
#include "ECSManager.h"
#include "ComponentViewBase.h"
#include "Scene.h"

using namespace std;

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
	
	~ComponentView();
	
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
	
	/// Applies func for every component in the view.
	/// \param func lambdaFunction
	void Foreach(void (*func)(ComponentTypes &...));
	
	/// Applies func for every component in the view using all possible threads. Note that only functions that
	/// only modify the current component are legal to use in parallel_foreach
	/// \param func
	void Parallel_foreach(std::function<void(ComponentTypes &...)> func, int minSize = 256);
	
	size_t Size();

protected:
	/// Checks if this ComponentView is interested in the given ComponentID meaning
	/// whether or not it uses this componentType
	/// \param type
	/// \return
	bool IsInterested(ComponentId type);

protected:
	UpdateType _currUpdateMode{UpdateType::Automatic}; // TODO: Implement function to change the way views are updated
	
	ECSManager &_manager;
	
	/// The ComponentIds of the types the ComponentView is interested in
	vector<ComponentId> _operatingTypes;
	
	/// Maps EntityID to the indices of its components in the respective ComponentVectors
	tsl::robin_map<EntityID, IndexType> *_registeredEntities{};
	
	vector<vector<IndexType>> *_vectoredEntities{};
	
	mutable std::condition_variable suspendCV{};
};


template<typename... ComponentTypes>
ComponentView<ComponentTypes...>::ComponentView()
		:
		ComponentView(Scene::activeScene->manager)
{
	assert(Scene::activeScene != nullptr && "No active scene was found!");
}

template<typename... ComponentTypes>
ComponentView<ComponentTypes...>::ComponentView(ECSManager &manager)
		:_manager(manager)
{
	_operatingTypes = {TypeId<ComponentTypes>::GetId()...};
	
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
	
	vector<IndexType> entityIndices{_manager.GetComponents<ComponentTypes>()->IndexOf(id)...};
	
	
	_vectoredEntities->push_back(entityIndices);
	_registeredEntities->insert(pair(id, _vectoredEntities->size() - 1));
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
	
	
	if (componentVectorIndex == _vectoredEntities->size() - 1) // special case when we remove the last thing
	{
		_vectoredEntities->pop_back();
	} else
	{
		// swap last and to be deleted position
		(*_vectoredEntities)[componentVectorIndex] = (*_vectoredEntities)[_vectoredEntities->size() - 1];
		_vectoredEntities->pop_back();
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
void ComponentView<ComponentTypes...>::Foreach(std::function<void(ComponentTypes &...)> func)
{
	// push back indices in case there is another upper foreach that called this foreach
	int componentIndex = 0;
	((ComponentVector<ComponentTypes>::index = componentIndex,
			ComponentVector<ComponentTypes>::savedIndices.push_back(componentIndex++))
			, ...);
	
	
	// apply function
	for (const vector<IndexType> &currVec : (*_vectoredEntities))
	{
		func(
				((*(_manager.GetComponents<ComponentTypes>()))
				[currVec[ComponentVector<ComponentTypes>::index]])...
		);
	}
	
	
	// pop back indices for the upper foreach
	((ComponentVector<ComponentTypes>::index = ComponentVector<ComponentTypes>::savedIndices.back(),
			ComponentVector<ComponentTypes>::savedIndices.pop_back())
			, ...);
}

template<typename... ComponentTypes>
void ComponentView<ComponentTypes...>::Foreach(void (*func)(ComponentTypes &...))
{
	// push back indices in case there is another upper foreach that called this foreach
	int componentIndex = 0;
	((ComponentVector<ComponentTypes>::index = componentIndex,
			ComponentVector<ComponentTypes>::savedIndices.push_back(componentIndex++))
			, ...);
	
	
	// apply function
	for (const vector<IndexType> &currVec : (*_vectoredEntities))
	{
		func(
				((*(_manager.GetComponents<ComponentTypes>()))
				[currVec[ComponentVector<ComponentTypes>::index]])...
		);
	}
	
	
	// pop back indices for the upper foreach
	((ComponentVector<ComponentTypes>::index = ComponentVector<ComponentTypes>::savedIndices.back(),
			ComponentVector<ComponentTypes>::savedIndices.pop_back())
			, ...);
}

template<typename... ComponentTypes>
void
ComponentView<ComponentTypes...>::Parallel_foreach(std::function<void(ComponentTypes &...)> func, const int minSize)
{
	// check if it actually makes sense to use parallel execution, depending on the input size
	size_t vectorSize = _vectoredEntities->size();
	
	if (vectorSize < minSize)
	{
		// use single core implementation
		Foreach(func);
		return;
	}
	
	// use multi threaded implementation
	int nThreads = static_cast<int>(std::thread::hardware_concurrency());
	atomic_int nDone = 0;
	
	std::mutex mutex;
	std::unique_lock<std::mutex> lock(mutex);
	
	int componentIndex = 0;
	((ComponentVector<ComponentTypes>::index = componentIndex++), ...);
	
	
	for (int j = 0; j < nThreads; ++j)
	{
		int start = j * (vectorSize / nThreads);
		int end = std::min((j + 1) * (vectorSize / nThreads), vectorSize);
		
		tPool.push([&func, start, end, &nDone, this](int id)
		           {
			           for (int i = start; i < end; ++i)
			           {
				           auto &currVec = (*_vectoredEntities)[i];
				
				           func(
						           ((*(_manager.GetComponents<ComponentTypes>()))
						           [currVec[ComponentVector<ComponentTypes>::index]])...
				           );
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
	delete _vectoredEntities;
	delete _registeredEntities;
	
	// init data structures
	_vectoredEntities = new vector<vector<IndexType>>();
	_vectoredEntities->reserve(16);
	
	_registeredEntities = new tsl::robin_map<EntityID, IndexType>();
	
	
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
		
		for (const std::pair<const EntityID, IndexType> &currPair : startComponents->getEntities())
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
			_vectoredEntities->push_back(componentIndices);
			_registeredEntities->insert(std::pair(currPair.first, _vectoredEntities->size() - 1));
		}
	}
}

template<typename... ComponentTypes>
ComponentView<ComponentTypes...>::~ComponentView()
{
	delete _vectoredEntities;
	delete _registeredEntities;
}