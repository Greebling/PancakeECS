// Created on 05.02.2020.

#include "Scene.h"

Scene *Scene::activeScene = nullptr;

Scene::Scene()
        : manager() {
    if (activeScene == nullptr)
        activeScene = this;
}

Scene::~Scene() {
    if (this == activeScene)
        activeScene = nullptr;
}

void Scene::SetActive() {
    activeScene = this;
}
