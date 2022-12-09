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
    //I have no idea why this is correct. If someone could tell me that would be great. -w
    stats.angle = scene_camera->transform->get_world_rotation() * glm::vec3(0.0f, 0.0f, -1.0f);
    stats.focal_distance = cur_focus;

    //get fragment counts for each drawable
    scene.render_picture(*scene_camera, stats.frag_counts, *stats.data);

    //sort by frag count
    auto sort_by_frag_count = [&](std::pair<Scene::Drawable &, GLuint> a, std::pair<Scene::Drawable &, GLuint> b) {
        return a.second > b.second;
    };
    stats.frag_counts.sort(sort_by_frag_count);
    stats.total_frag_count = 0;
    std::for_each(stats.frag_counts.begin(), stats.frag_counts.end(), [&](auto pair) {
        stats.total_frag_count += pair.second;
    });

    //Set of creatures in frame, because there will be duplicates for body parts
    std::set< std::string > creature_set;
    for (auto &pair : stats.frag_counts) {
        if((float)pair.second/(float)stats.total_frag_count > 0.0012f) { //don't count tiny amounts of frags
            std::string name = pair.first.transform->name;
            std::string code_id = name.substr(0, 6);
            if (Creature::creature_map.count(code_id)) {
                creature_set.insert(code_id);
            } else if (code_id.substr(0, 3) == "PLT") {
                stats.plant_set.insert(code_id);
            }
//            std::cout << pair.first.transform->name << "in: " << (float)pair.second << std::endl;
        } else {
//            std::cout << pair.first.transform->name << " out: " << (float)pair.second << std::endl;
        }
    }

    //list of creatures & focal point visibilities
    for (auto &code_id : creature_set) {
        stats.creatures_in_frame.emplace_back();
        stats.creatures_in_frame.back().creature = &Creature::creature_map[code_id];
    }

    // Populate creature infos
    for (auto &creature_info : stats.creatures_in_frame) {
        // Get focal point vector - indices of bools map to indices of focal points in creature
        scene.test_focal_points(*scene_camera, creature_info.creature->focal_points, creature_info.are_focal_points_in_frame);
        // Populate frag counts by summing over all labeled parts
        creature_info.frag_count = 0;
        std::for_each(stats.frag_counts.begin(), stats.frag_counts.end(), [&](auto pair) {
            if (pair.first.transform->name.substr(0, 6)
                    == creature_info.creature->get_code_and_number()) {
            creature_info.frag_count += pair.second;
            }
        });
        // Vector from player to creature
        glm::vec3 worldpos = creature_info.creature->transform->make_local_to_world()[3];
        glm::vec3 camera_worldpos = player->player_camera->scene_camera->transform->make_local_to_world()[3];
        //std::cout << glm::to_string(creature_info.creature->transform->make_local_to_world()) <<std::endl;
        //std::cout << glm::to_string(glm::vec4(creature_info.creature->transform->position, 1.0f)) <<std::endl;
        //std::cout << glm::to_string(camera_worldpos) <<std::endl;
        //std::cout << glm::to_string(worldpos) <<std::endl;
        creature_info.player_to_creature = worldpos - camera_worldpos;
        //std::cout << glm::to_string(creature_info.player_to_creature) <<std::endl;
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

	auto temp = std::make_shared<Picture>(stats);

    player->pictures.push_back(temp);
    std::shared_ptr<Picture> picture = player->pictures.back();
    //update creature stats map
    if (picture->subject_info.creature) {
        Creature::creature_stats_map.at(picture->subject_info.creature->code).on_picture_taken(picture);
    }
	std::cout << picture->get_scoring_string() << std::endl;

	cur_battery -= 1;
}

// Akin to adjusting length of lens; affects focus (zooming in makes depth of field shallower)
void PlayerCamera::AdjustZoom(bool increase) {

	float diff = default_zoom_increment;
	if (!increase) diff *= -1.0;

	if (cur_zoom + diff > max_zoom || cur_zoom + diff < min_zoom) {
		return;
	}

	cur_zoom += diff;
	scene_camera->fovy = default_fovy / cur_zoom;

	// Adjusting zoom (lens length) also impacts depth of field
	this->AdjustFocus(!increase);
}

void PlayerCamera::AdjustFocus(bool increase) {

	float diff = increase ? 1.0f : -1.0f;

	// Increment of adjustment increases as focal distance increases
	// TODO: Make this not hardcoded based on the specific min/max values
	if (cur_focus < 1.1f) {
		// increments of 0.1 for 0.1 to 1
		diff *= 0.1f;
	}
	else if (cur_focus >= 1.1f && cur_focus <= 5.0f) {
		// increments of 0.25 for 1.1 to 5
		diff *= 0.25f;
	}
	else if (cur_focus > 5.0f && cur_focus <= 10.0f) {
		// increments of 0.5 for 5 to 10
		diff *= 0.5f;
	}
	else if (cur_focus > 10.0f && cur_focus <= 20.0f) {
		// increments of 1 for 10 to 20
		diff *= 1.0f;
	}
	else if (cur_focus > 20.0f) {
		// increments of 2 for above 20
		diff *= 2.0f;
	}

	if (cur_focus + diff > max_focus || cur_focus + diff < min_focus) {
		return;
	}

    cur_focus += diff;
}

void PlayerCamera::Reset(bool reset_battery) {

	cur_focus = default_focus;
	cur_zoom = default_zoom;
	scene_camera->fovy = default_fovy / cur_zoom;

	if (reset_battery) {
		cur_battery = max_battery;
	}
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

    pictures = std::vector<std::shared_ptr<Picture>>();
}

Player::~Player() {
	delete player_camera;
}

// Clear all pictures taken at the end of the day. Saved pictures are store in a list in Playmode and best pictures for each creature are stored with each creature
void Player::ClearPictures() {
	pictures.clear();
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
    Sound::listener.set_position_right(camera->transform->make_local_to_world()[3], camera->transform->get_world_rotation() * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
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
    Sound::listener.set_position_right(camera->transform->make_local_to_world()[3], camera->transform->get_world_rotation() * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
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

void Player::SetRun(bool _is_running) {
	if (_is_running) {
		speed = run_speed;
	}
	else {
		speed = walk_speed;
	}
	is_running = _is_running;
}

float Player::get_speed() {

    if (in_cam_view) {
        return speed / 2.0f;
    } 
	else {
        return speed;
    }
}