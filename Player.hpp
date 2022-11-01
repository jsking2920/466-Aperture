#pragma once

#include "WalkMesh.hpp"
#include "Scene.hpp"
#include "Picture.hpp"

#include <glm/glm.hpp>
#include <memory>

struct Player;

// Cameras for taking pictures
struct PlayerCamera {

	// Newly created transform for this camera, transform of parent (should be player.camera)
	PlayerCamera(Scene::Transform* scene_transform);
	~PlayerCamera();

	Player* player = nullptr; // initialized by Player contstructor
	Scene::Camera* scene_camera; // used for actually drawing view of scene

	void TakePicture(Scene &scene); // Adds picture to player.pictures
    Picture GeneratePicture(std::list<std::pair<Scene::Drawable &, GLuint>> frag_counts);
};

struct Player {

	Player(Scene::Transform* _transform, WalkMesh const* _walk_mesh, Scene::Camera* _camera, Scene::Transform* _player_camera_transform);
	~Player();

	// Transform of the player mesh in the scene; will be yawed by mouse left/right motion
	Scene::Transform* transform;

	WalkMesh const* walk_mesh;
	WalkPoint at;

	float speed = 6.0f;
	float crouch_offset = 0.8f;
	bool is_crouched = false;
	
	// Camera in scene, at head of player; will be pitched by mouse up/down motion ("Eyes" of player)
	Scene::Camera* camera;
	// Camera in player's hands that they take picures with parented to camera
	PlayerCamera* player_camera;
	// false = view from "eyes"/camera, true = view from PlayerCamera "viewport" (picture taking mode)
	bool in_cam_view = false;
	// List of pics player has taken
	std::list<Picture>* pictures = nullptr; // TODO: add an album view in game to look at these
	
	void ToggleCrouch(); // Adjusts z pos of player's cameras so it looks like they crouched
	void Move(glm::vec2 direction, float elapsed); // un-normalized, cardinal directions such as (-1.0f, 0.0f) -> left button only held, (1.0f, 1.0f) -> right and up buttons held, etc. 
	void OnMouseMotion(glm::vec2 mouse_motion);
};

