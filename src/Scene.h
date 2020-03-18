// Created on 05.02.2020.

#pragma once


#include "ECSManager.h"

class Scene {
public:
    static Scene *activeScene;

    ECSManager manager;

public:
    Scene();

    ~Scene();

    /// Marks this scene as the currently active one
    void SetActive();
};
