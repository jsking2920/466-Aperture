#include "Player.hpp"
#include "load_save_png.hpp"
#include "data_path.hpp"
#include "Framebuffers.hpp"

#include <iostream>
#include <algorithm>

PlayerCamera::PlayerCamera(Scene::Transform* scene_transform, Scene::Transform* parent_transform, float fovy) {

	scene_camera = std::make_unique<Scene::Camera>(scene_transform);

	// Player's camera shares same position/rotation/etc as their scene camera ("eyes")
	scene_camera->transform->parent = parent_transform; // parent_transform should be player.transform

	// Set default camera settings
	scene_camera->fovy = fovy;
	scene_camera->near = 0.01f;
}

PlayerCamera::~PlayerCamera() {
}

void PlayerCamera::TakePicture(Scene &scene, std::list<Picture> &pictures) {
    //get fragment counts for each drawable
    std::list<std::pair<Scene::Drawable &, GLuint>> result;

    GLfloat *data = new GLfloat[3 * scene_camera->drawable_size.x * scene_camera->drawable_size.y];
    scene.render_picture(*scene_camera, result, data);

    Picture picture = GeneratePicture(result);
    pictures.push_back(picture);
    std::cout << picture.get_scoring_string() << std::endl;


    //convert pixel data to correct format for png export
    uint8_t *png_data = new uint8_t [4 * scene_camera->drawable_size.x * scene_camera->drawable_size.y];
    for(uint32_t i = 0; i < scene_camera->drawable_size.x * scene_camera->drawable_size.y; i++) {
        png_data[i * 4    ] = (uint8_t)round(data[i * 3    ] * 255);
        png_data[i * 4 + 1] = (uint8_t)round(data[i * 3 + 1] * 255);
        png_data[i * 4 + 2] = (uint8_t)round(data[i * 3 + 2] * 255);
        png_data[i * 4 + 3] = 255;
    }
    save_png(data_path("album/" + picture.title + ".png"), scene_camera->drawable_size,
             reinterpret_cast<const glm::u8vec4 *>(png_data), LowerLeftOrigin);

    free(data);
}

Picture PlayerCamera::GeneratePicture(std::list<std::pair<Scene::Drawable &, GLuint>> frag_counts) {
    Picture picture = Picture();
    if(frag_counts.empty()) {
        picture.title = "Pure Emptiness";
        picture.score_elements.emplace_back("Relatable", 500);
        picture.score_elements.emplace_back("Deep", 500);
        return picture;
    }
    auto sort_by_frag_count = [&](std::pair<Scene::Drawable &, GLuint> a, std::pair<Scene::Drawable &, GLuint> b) {
        return a.second > b.second;
    };
    frag_counts.sort(sort_by_frag_count);

    //TODO: improve subject selection process
    Scene::Drawable &subject = frag_counts.front().first;
    unsigned int subject_frag_count = frag_counts.front().second;

    //Add points for bigness
    picture.score_elements.emplace_back("Bigness", subject_frag_count/1000);

    //Add bonus points for additional subjects
    std::for_each(std::next(frag_counts.begin()), frag_counts.end(), [&](std::pair<Scene::Drawable &, GLuint> creature) {
        picture.score_elements.emplace_back("Bonus " + creature.first.transform->name, 100);
    });

    //Magnificence
    picture.score_elements.emplace_back("Magnificence", 200);

    //TODO: make name unique for file saving purposes
    picture.title = "Magnificent " + subject.transform->name;

    return picture;
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
