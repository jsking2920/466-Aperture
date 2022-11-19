#include "Player.hpp"
#include "load_save_png.hpp"
#include "data_path.hpp"
#include "Framebuffers.hpp"
#include "GameObjects.hpp"
#include "glm/gtx/string_cast.hpp"
#include "Sound.hpp"

#include <iostream>
#include <algorithm>
#include <map>
#include <set>
#include <filesystem>

// PlayerCamera
//========================================

PlayerCamera::PlayerCamera(Scene::Transform* scene_transform) {

	scene_camera = new Scene::Camera(scene_transform);

	// Set default camera settings
	scene_camera->fovy = default_fovy;
	scene_camera->near = 0.01f;
}

PlayerCamera::~PlayerCamera() {
	delete scene_camera;
}

void PlayerCamera::TakePicture(Scene &scene) {
    Sound::play(Sound::sample_map->at("CameraClick"));

    PictureInfo stats;
    stats.data = std::make_shared<std::vector<GLfloat>>(3 * scene_camera->drawable_size.x * scene_camera->drawable_size.y);
    stats.dimensions = scene_camera->drawable_size;
    stats.angle = eulerAngles(player->camera->transform->rotation);

    //get fragment counts for each drawable
    scene.render_picture(*scene_camera, stats.frag_counts, *stats.data);

    //sort by frag count
    auto sort_by_frag_count = [&](std::pair<Scene::Drawable &, GLuint> a, std::pair<Scene::Drawable &, GLuint> b) {
        return a.second > b.second;
    };
    stats.frag_counts.sort(sort_by_frag_count);
    std::for_each(stats.frag_counts.begin(), stats.frag_counts.end(), [&](auto pair) {
        stats.total_frag_count += pair.second;
    });

    //Set of creatures in frame, because there will be duplicates for body parts
    std::set< std::string > creature_set;
    for (auto &pair : stats.frag_counts) {
        std::string name = pair.first.transform->name;
        std::string code_id = name.substr(0, 6);
        if (Creature::creature_map.count(code_id)) {
            creature_set.insert(code_id);
        }
    }

    //list of creatures & focal point visibilities
    for (auto &code_id : creature_set) {
        stats.creatures_in_frame.emplace_back();
        stats.creatures_in_frame.back().creature = &Creature::creature_map[code_id];
    }

    //populate creature infos
    for (auto &creature_info : stats.creatures_in_frame) {
        //Get focal point vector - indices of bools map to indices of focal points in creature
        scene.test_focal_points(*scene_camera, creature_info.creature->focal_points, creature_info.are_focal_points_in_frame);
        //populate frag counts by summing over all labeled parts
        creature_info.frag_count = 0;
        std::for_each(stats.frag_counts.begin(), stats.frag_counts.end(), [&](auto pair) {
            if (pair.first.transform->name.substr(0, 6)
                    == creature_info.creature->get_code_and_number()) {
            creature_info.frag_count += pair.second;
            }
        });
        //vector from player to creature
        glm::vec3 worldpos = creature_info.creature->transform->make_local_to_world() * glm::vec4(creature_info.creature->transform->position, 1.0f);
        glm::vec3 camera_worldpos = player->camera->transform->make_local_to_world() * glm::vec4(player->camera->transform->position, 1.0f);
        creature_info.player_to_creature = worldpos - camera_worldpos;
        //std::cout << glm::to_string(worldpos) << ",  " << glm::to_string(camera_worldpos);
    }

    // Debug: print how many focal points are in frame
	/*
    for (auto &pair : creatures_in_frame) {
        auto pred = [&](bool b) {
            return b;
        };
        std::cout << pair.first->name << " detected, focal point count: " << std::count_if((*pair.second).begin(),
                                                                                           (*pair.second).end(), pred) << std::endl;
    }
	*/

    player->pictures->emplace_back(stats);
    Picture &picture = player->pictures->back();
	std::cout << picture.get_scoring_string() << std::endl;

	// TODO: move save picture out of here to make it user-prompted
    picture.save_picture_png();

	cur_battery -= 1;
}

void PlayerCamera::AdjustZoom(float diff) {

	float new_zoom = cur_zoom + diff;
	if (new_zoom < min_zoom - 0.001f || new_zoom > max_zoom + 0.001f) {
		return;
	}
	cur_zoom = new_zoom;

	scene_camera->fovy = default_fovy / cur_zoom;
}

// Player
//========================================

Player::Player(Scene::Transform* _transform, WalkMesh const* _walk_mesh, Scene::Camera* _camera, Scene::Transform* _player_camera_transform) {
	
	transform = _transform;
	walk_mesh = _walk_mesh;
	camera = _camera;
	
	camera->transform->parent = transform;
	camera->fovy = 3.14159265358979323846f / 3.0f; // 60 degree vertical fov
	camera->near = 0.01f;

	base_cam_z = camera->transform->position.z;
	
	// Start player walking at nearest walk point
	at = walk_mesh->nearest_walk_point(transform->position);
	
	player_camera = new PlayerCamera(_player_camera_transform);
	player_camera->player = this;
	// Player's camera shares same position/rotation/etc as their scene camera ("eyes")
	player_camera->scene_camera->transform->parent = camera->transform;
	player_camera->scene_camera->fovy = camera->fovy;

    pictures = new std::list<Picture>();
}

Player::~Player() {
	delete player_camera;
    delete pictures;
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
	if (direction != glm::vec2(0.0f)) direction = glm::normalize(direction) * get_speed() * elapsed;

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

void Player::SetCrouch(bool _is_crouched) {

	if (_is_crouched) {
		camera->transform->position.z = base_cam_z - crouch_offset;
	}
	else {
		camera->transform->position.z = base_cam_z;
	}
	is_crouched = _is_crouched;
}

float Player::get_speed() {

    if (in_cam_view) {
        return speed / 2.0f;
    } 
	else {
        return speed;
    }
}