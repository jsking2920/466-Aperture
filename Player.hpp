#pragma once

#include "WalkMesh.hpp"
#include "Scene.hpp"
#include "Picture.hpp"
#include "GameObjects.hpp"

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

	float default_fovy = 3.14159265358979323846f / 3.0f; // 60 degree vertical fov
	float cur_zoom = 1.0f;
	float default_zoom = 1.0f;
	float default_zoom_increment = 0.1f; // increment per frame while scrolling mouse up or down
	float min_zoom = 0.5f;
	float max_zoom = 3.0f;

    float cur_focus = 3.0f;
	float default_focus = 3.0f;
    float min_focus = 0.1f;
    float max_focus = 40.f;
	
	// Measured in number of possible pictures that can be taken, displayed as a percentage
	uint8_t cur_battery = 20;
	uint8_t max_battery = 20;

	void TakePicture(Scene &scene); // Adds picture to player.pictures
	void AdjustZoom(bool increase); // zooms in if increase == true; also affects focus
    void AdjustFocus(bool increase);
	void Reset(bool reset_battery); // resets zoom, focus, and battery to defaults if desired
};

struct Player {

	Player(Scene::Transform* _transform, WalkMesh const* _walk_mesh, Scene::Camera* _camera, Scene::Transform* _player_camera_transform);
	~Player();

	// Transform of the player mesh in the scene; will be yawed by mouse left/right motion
	Scene::Transform* transform;

	WalkMesh const* walk_mesh;
	WalkPoint at;

	float walk_speed = 6.0f;
	float run_speed = 9.0f;
	float speed = 6.0f;

	float base_cam_z;
	float crouch_offset = 0.8f;
	bool is_crouched = false;
	bool is_running = false;
	
	// Camera in scene, at head of player; will be pitched by mouse up/down motion ("Eyes" of player)
	Scene::Camera* camera;
	// Camera in player's hands that they take picures with parented to camera
	PlayerCamera* player_camera;
	// false = view from "eyes"/camera, true = view from PlayerCamera "viewport" (picture taking mode)
	bool in_cam_view = false;
	// List of pics player has taken
	std::vector<std::shared_ptr<Picture>> pictures;
	
	void SetCrouch(bool is_crouched); // Adjusts z pos of player's cameras so it looks like they crouched
	void SetRun(bool _is_running); // Adjusts speed of player
	void Move(glm::vec2 direction, float elapsed); // un-normalized, cardinal directions such as (-1.0f, 0.0f) -> left button only held, (1.0f, 1.0f) -> right and up buttons held, etc. 
	void OnMouseMotion(glm::vec2 mouse_motion);
    float get_speed(); //for halving speed in cam mode
	void ClearPictures();
};

