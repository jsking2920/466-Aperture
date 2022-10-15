#pragma once

#include "WalkMesh.hpp"
#include "Scene.hpp"

#include <glm/glm.hpp>

class Player {

public:
	WalkMesh const* walk_mesh = nullptr;
	WalkPoint at;

	// Transform of the player mesh in the scene; will be yawed by mouse left/right motion
	Scene::Transform* transform = nullptr;

	// Camera in scene, at head of player; will be pitched by mouse up/down motion
	Scene::Camera* camera = nullptr;

	float speed = 6.0f;

	void Move(glm::vec2 direction, float elapsed); // un-normalized, cardinal directions such as (-1.0f, 0.0f) -> left button only held, (1.0f, 1.0f) -> right and up buttons held, etc. 
	void OnMouseMotion(glm::vec2 mouse_motion);

private:

};

