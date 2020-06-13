#pragma once

#include <cassert>
#include "EntityID.h"
#include "ECSManager.h"
#include "Scene.h"


/// A wrapper class for an Entity
/// Has convenient methods for adding and removing components and takes care of their disposal automatically
/// May be inherited from
class GameObject
{
public:
	/// Creates a GameObject within the current scene
	GameObject()
			: scene(*Scene::ACTIVE_SCENE)
			  , manager(Scene::ACTIVE_SCENE->manager)
	{
	}
	
	virtual ~GameObject()
	{
		if (_id.IsAlive())
			manager.DestroyEntity(_id);
	}
	
	void Spawn(Scene &spawningScene = *Scene::ACTIVE_SCENE)
	{
		assert(!_id.IsAlive());
		
		scene = spawningScene;
		manager = scene.manager;
		_id = manager.AddEntity();
		
		OnSpawn();
	}
	
	
	/// The entityID of the GameObject
	/// \return
	[[nodiscard]] EntityID Id() const
	{
		return _id;
	}
	
	
	/// Returns the given component of the GameActor
	/// \tparam ComponentType
	/// \return A pointer to the component
	template<typename ComponentType>
	ComponentHandle<ComponentType> Component()
	{
		static_assert(std::is_base_of_v<ComponentData, ComponentType>,
		              "ComponentType has to derive from ComponentData!");
		
		assert(_id.IsAlive()); // GameObject has not been spawned!
		
		return manager.GetComponent<ComponentType>(_id);
	}
	
	
	/// Adds the given ComponentType that belongs to the given id
	/// \tparam ComponentType Deriving from ComponentData
	/// \param id Owner of the Component
	/// \return A pointer to the component
	template<typename ComponentType>
	ComponentHandle<ComponentType> AddComponent()
	{
		assert(_id.IsAlive()); // GameObject has not been spawned!
		return manager.AddComponent<ComponentType>(_id);
	}

protected:
	virtual void OnSpawn()
	{
	}

protected:
	/// The ECS Manager the GameObject is bound to
	ECSManager &manager;
	
	/// The scene the GameObject lives in
	Scene &scene;

private:
	EntityID _id;
};