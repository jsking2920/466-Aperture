#include "Player.hpp"

#include <iostream>

PlayerCamera::PlayerCamera(Scene::Camera* camera) {
	scene_camera = camera;
}

PlayerCamera::~PlayerCamera() {
}

void Player::OnMouseMotion(glm::vec2 mouse_motion) {

	// Player rotation locked to z-axis rotation
	transform->rotation = glm::angleAxis(-mouse_motion.x * camera->fovy, glm::vec3(0.0f, 0.0f, 1.0f)) * transform->rotation;

	float pitch = mouse_motion.y * camera->fovy;
	// Constrain pitch to nearly straight down and nearly straight up
	float end_pitch = glm::pitch(camera->transform->rotation) + pitch;
	if (end_pitch > 0.95f * 3.1415926f || end_pitch < 0.05f * 3.1415926f) {
		pitch = 0.0f;
	}
	camera->transform->rotation = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f)) * camera->transform->rotation;
}

void Player::Move(glm::vec2 direction, float elapsed) {

	// Make it so that moving diagonally doesn't go faster
	if (direction != glm::vec2(0.0f)) direction = glm::normalize(direction) * speed * elapsed;

	// Get move in world coordinate system
	glm::vec3 remain = transform->make_local_to_world() * glm::vec4(direction.x, direction.y, 0.0f, 0.0f);

	// using a for() instead of a while() here so that if walkpoint gets stuck in
	// some awkward case, code will not infinite loop
	for (uint32_t iter = 0; iter < 10; ++iter) {
		if (remain == glm::vec3(0.0f)) break;
		WalkPoint end;
		float time;
		walk_mesh->walk_in_triangle(at, remain, &end, &time);
		at = end;
		if (time == 1.0f) {
			// finished within triangle:
			remain = glm::vec3(0.0f);
			break;
		}
		// some step remains:
		remain *= (1.0f - time);
		// try to step over edge:
		glm::quat rotation;
		if (walk_mesh->cross_edge(at, &end, &rotation)) {
			// stepped to a new triangle:
			at = end;
			// rotate step to follow surface:
			remain = rotation * remain;
		}
		else {
			// ran into a wall, bounce / slide along it:
			glm::vec3 const& a = walk_mesh->vertices[at.indices.x];
			glm::vec3 const& b = walk_mesh->vertices[at.indices.y];
			glm::vec3 const& c = walk_mesh->vertices[at.indices.z];
			glm::vec3 along = glm::normalize(b - a);
			glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
			glm::vec3 in = glm::cross(normal, along);

			//check how much 'remain' is pointing out of the triangle:
			float d = glm::dot(remain, in);
			if (d < 0.0f) {
				//bounce off of the wall:
				remain += (-1.25f * d) * in;
			}
			else {
				//if it's just pointing along the edge, bend slightly away from wall:
				remain += 0.01f * d * in;
			}
		}
	}

	if (remain != glm::vec3(0.0f)) {
		std::cout << "NOTE: code used full iteration budget for walking." << std::endl;
	}

	// Update player's position to respect walking
	transform->position = walk_mesh->to_world_point(at);
}
