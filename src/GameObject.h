// Created on 19.12.2019.

#pragma once

#include <cassert>
#include "EntityID.h"
#include "ECSManager.h"
#include "Scene.h"


class GameObject {
public:
    std::string name;

protected:
    ECSManager &manager;

private:
    const EntityID _id;


public:
    /// Creates a GameObject within the current scene
    GameObject()
            : manager(Scene::activeScene->manager),
              _id(Scene::activeScene->manager.AddEntity()) {
        assert(Scene::activeScene != nullptr && "A scene must be active before instantiating a GameObject");
    }

    explicit GameObject(Scene &scene)
            : manager(scene.manager), _id(scene.manager.AddEntity()) {
    }

    virtual ~GameObject();


    /// The entityID of the actor
    /// \return
    [[nodiscard]] EntityID Id() const;


    /// Returns the given component of the GameActor
    /// \tparam ComponentType
    /// \return A pointer to the component
    template<typename ComponentType>
    ComponentPointer<ComponentType> Component();


    /// Adds the given ComponentType that belongs to the given id
    /// \tparam ComponentType Deriving from ComponentData
    /// \param id Owner of the Component
    /// \return A pointer to the component
    template<typename ComponentType>
    ComponentPointer<ComponentType> AddComponent();
};


template<typename ComponentType>
ComponentPointer<ComponentType> GameObject::Component() {
    static_assert(std::is_base_of_v<ComponentData, ComponentType>,
                  "ComponentType has to derive from ComponentData!");

    return manager.GetComponent<ComponentType>(_id);
}

template<typename ComponentType>
ComponentPointer<ComponentType> GameObject::AddComponent() {
    return manager.AddComponent<ComponentType>(_id);
}