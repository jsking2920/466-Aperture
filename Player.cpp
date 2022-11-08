#include "Player.hpp"
#include "load_save_png.hpp"
#include "data_path.hpp"
#include "Framebuffers.hpp"
#include "GameObjects.hpp"

#include <iostream>
#include <algorithm>
#include <map>
#include <unordered_set>
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

PlayerCamera::PictureStatistics::PictureStatistics(PlayerCamera camera) {
    data = std::make_shared<GLfloat>(3 * camera.scene_camera->drawable_size.x * camera.scene_camera->drawable_size.y);

}

void PlayerCamera::TakePicture(Scene &scene) {

    PictureStatistics stats(*this);

    //get fragment counts for each drawable
    scene.render_picture(*scene_camera, stats.frag_counts, stats.data);

    //Set of creatures in frame, because there will be duplicates for body parts
    std::unordered_set< std::string > creature_set;
    for (auto &pair : stats.frag_counts) {
        std::string name = pair.first.transform->name;
        std::string code = name.substr(0, name.find('-'));
        if (Creature::creature_map.count(code)) {
            creature_set.insert(code);
        }
    }

    //list of creatures & focal point visibilities
    //i know this type is ugly af but i think it's good for memory management bc of vector resizes. i think.
//    std::list< std::pair<Creature *, std::shared_ptr< std::vector< bool > > > > creatures_in_frame;
    for (auto &code : creature_set) {
        stats.creatures_in_frame.emplace_back(std::make_pair(&Creature::creature_map[code], std::make_shared<std::vector< bool >>()));
    }

    //Get focal point vector - indices of bools map to indices of focal points in creature
    for (auto &pair : stats.creatures_in_frame) {
        scene.test_focal_points(*scene_camera, pair.first->focal_points, *pair.second);
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

    // TODO: i think we should make a struct for picture info, there's so much info we can pass in
    Picture picture = ScorePicture(stats);
    player->pictures->push_back(picture);
	//std::cout << picture.get_scoring_string() << std::endl;
	// 
	// TODO: move save picture out of here to make it user-prompted; will need to handle data/memory allocation differently
	SavePicture(stats.data, picture.title);
}

Picture PlayerCamera::ScorePicture(PlayerCamera::PictureStatistics stats) {

    Picture picture = Picture();

    if (stats.frag_counts.empty()) {
        picture.title = "Pure Emptiness";
        picture.score_elements.emplace_back("Relatable", 500);
        picture.score_elements.emplace_back("Deep", 500);
        return picture;
    }

    auto sort_by_frag_count = [&](std::pair<Scene::Drawable &, GLuint> a, std::pair<Scene::Drawable &, GLuint> b) {
        return a.second > b.second;
    };
    stats.frag_counts.sort(sort_by_frag_count);

    //TODO: improve subject selection process
    Scene::Drawable &subject = stats.frag_counts.front().first;
    unsigned int subject_frag_count = stats.frag_counts.front().second;

    //Add points for bigness
    picture.score_elements.emplace_back("Bigness", subject_frag_count/1000);

    //Add bonus points for additional subjects
    std::for_each(std::next(stats.frag_counts.begin()), stats.frag_counts.end(), [&](std::pair<Scene::Drawable &, GLuint> creature) {
        picture.score_elements.emplace_back("Bonus " + creature.first.transform->name, 100);
    });

    //Magnificence
    picture.score_elements.emplace_back("Magnificence", 200);

    //TODO: make name unique for file saving purposes
    picture.title = "Magnificent " + subject.transform->name;

    return picture;
}

void PlayerCamera::SavePicture(std::shared_ptr<GLfloat[]> data, std::string name) {

	//convert pixel data to correct format for png export
	uint8_t* png_data = new uint8_t[4 * scene_camera->drawable_size.x * scene_camera->drawable_size.y];
	for (uint32_t i = 0; i < scene_camera->drawable_size.x * scene_camera->drawable_size.y; i++) {
		png_data[i * 4] = (uint8_t)round(data[i * 3] * 255);
		png_data[i * 4 + 1] = (uint8_t)round(data[i * 3 + 1] * 255);
		png_data[i * 4 + 2] = (uint8_t)round(data[i * 3 + 2] * 255);
		png_data[i * 4 + 3] = 255;
	}
	// create album folder if it doesn't exist
	if (!std::filesystem::exists(data_path("PhotoAlbum/"))) {
		std::filesystem::create_directory(data_path("PhotoAlbum/"));
	}
	save_png(data_path("PhotoAlbum/" + name + ".png"), scene_camera->drawable_size,
		reinterpret_cast<const glm::u8vec4*>(png_data), LowerLeftOrigin);
    delete[] png_data;
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

void Player::ToggleCrouch() {

	if (is_crouched) {
		camera->transform->position.z += crouch_offset;
	}
	else {
		camera->transform->position.z -= crouch_offset;
	}
	is_crouched = !is_crouched;
}