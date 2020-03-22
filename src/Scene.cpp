#include "Scene.h"

Scene *Scene::ACTIVE_SCENE = nullptr;

Scene::Scene()
		: manager()
{
	if (ACTIVE_SCENE == nullptr)
		ACTIVE_SCENE = this;
}

Scene::~Scene()
{
	if (this == ACTIVE_SCENE)
		ACTIVE_SCENE = nullptr;
}

void Scene::SetActive()
{
	ACTIVE_SCENE = this;
}
