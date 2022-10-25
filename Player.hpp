#pragma once

#include "WalkMesh.hpp"
#include "Scene.hpp"

#include <glm/glm.hpp>
#include <memory>

// Cameras for taking pictures
struct PlayerCamera {

	// Newly created transform for this camera, transform of parent (should be player.camera)
	PlayerCamera(Scene::Transform* scene_transform, Scene::Transform* parent_transform, float fovy);
	~PlayerCamera();

	std::unique_ptr<Scene::Camera> scene_camera; // used for actually drawing view of scene

	void TakePicture();
};

struct Player {

	// Transform of the player mesh in the scene; will be yawed by mouse left/right motion
	Scene::Transform* transform = nullptr;

	WalkMesh const* walk_mesh = nullptr;
	WalkPoint at;
	float speed = 6.0f;
	
	// Camera in scene, at head of player; will be pitched by mouse up/down motion ("Eyes" of player)
	Scene::Camera* camera = nullptr;
	// Camera in player's hands that they take picures with parented to camera
	std::unique_ptr<PlayerCamera> player_camera = nullptr;
	// false = view from "eyes"/camera, true = view from PlayerCamera "viewport" (picture taking mode)
	bool in_cam_view = false;

	
	void Move(glm::vec2 direction, float elapsed); // un-normalized, cardinal directions such as (-1.0f, 0.0f) -> left button only held, (1.0f, 1.0f) -> right and up buttons held, etc. 
	void OnMouseMotion(glm::vec2 mouse_motion);
};

