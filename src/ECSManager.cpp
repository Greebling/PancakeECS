//
// Created by B98 on 09.11.2019.
//

#include "ECSManager.h"

bool Entity::IsAlive() const {
    return id.IsAlive();
}


////////////////////////////////////////////////////////


Entity *ECSManager::GetEntity(EntityID id) {
    if (id.Salt() == 0 || id.Index() == 0) {
        return nullptr;
    }

    if (((*_entities)[id.Index()].id.Index()) != 0 &&
        (*_entities)[id.Index()].id.Salt() == id.Salt()) {
        return &(*_entities)[id.Index()];
    } else {
        return nullptr;
    }
}

EntityID ECSManager::AddEntity() {
    // check if we have indices saved in _deletedIndices
    IndexType insertIndex;

    if (!_deletedIndices->empty()) {
        insertIndex = _deletedIndices->front();
        _deletedIndices->pop_front();
    } else {
        insertIndex = _lastInsert;
        _lastInsert++;

        if (_lastInsert >= std::numeric_limits<IndexType>::max())
            _lastInsert = 1;
    }

    // check if _entities can hold the entity, else resize it
    if (_entities->size() <= insertIndex)
        _entities->resize(insertIndex + 1);


    // create id
    EntityID newID(insertIndex, ((*_entities)[insertIndex].id.Salt()) + 1);
    // assign to entity
    (*_entities)[insertIndex].id = newID;

    // return the created id
    return newID;
}

bool ECSManager::DestroyEntity(EntityID id) {
    Entity *pEntity = GetEntity(id);

    if (pEntity != nullptr) {
        // Notify all component systems on the removal of the entity and all
        // possible components that the entity could have
        for (auto &compSysVector : _componentSystems) {
            NotifyOnRemove(compSysVector.first, id);
        }

        // Remove all components of entity from componentVectors
        for (auto &compVectorPair : _componentVectors) {
	        compVectorPair.second->RemoveComponentFrom(id);
        }

	    pEntity->id.MarkDead();


        _deletedIndices->push_back(id.Index());

        return true;
    }
    return false;
}

ECSManager::~ECSManager() {
    for (auto ComponentVector : _componentVectors) {
        // delete the componentVector part
        delete ComponentVector.second;
    }
    delete _deletedIndices;
}

void ECSManager::RegisterComponentSystem(ComponentViewBase *system, const std::vector<ComponentId> &componentIds) {
    for (ComponentId currId : componentIds) {
        if (_componentSystems.count(currId)) {
            _componentSystems.at(currId).push_back(system);
        } else {
            std::vector<ComponentViewBase *> compSystems{system};
            _componentSystems.insert(std::pair(currId, compSystems));
        }
    }
}

void ECSManager::NotifyOnAdd(ComponentId componentType, EntityID id) {
    if (!_componentSystems.count(componentType))
        return;

    std::vector<ComponentViewBase *> interestedSystems = _componentSystems.at(componentType);
    for (ComponentViewBase *compSystem : interestedSystems) {
	    compSystem->OnComponentAdded(componentType, id);
    }
}

void ECSManager::NotifyOnRemove(ComponentId componentType, EntityID id) {
    if (!_componentSystems.count(componentType))
        return;

    std::vector<ComponentViewBase *> interestedSystems = _componentSystems.at(componentType);
    for (ComponentViewBase *compSystem : interestedSystems) {
	    compSystem->OnComponentRemoved(componentType, id);
    }
}

ComponentVectorBase *ECSManager::GetComponentsBase(ComponentId componentType) {
    if (!_componentVectors.count(componentType))
        return nullptr;

    return _componentVectors[componentType];
}


