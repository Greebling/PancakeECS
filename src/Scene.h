#pragma once


#include "ECSManager.h"

class Scene
{
public:
	static Scene *ACTIVE_SCENE;
	
	ECSManager manager;

public:
	Scene();
	
	~Scene();
	
	/// Marks this scene as the currently active one
	void SetActive();
};
