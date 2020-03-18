// Created on 19.12.2019.

#include "GameObject.h"

EntityID GameObject::Id() const {
    return _id;
}

GameObject::~GameObject() {
	manager.DestroyEntity(_id);
}
