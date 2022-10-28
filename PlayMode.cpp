#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <random>

GLuint main_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > main_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	std::cout << "Loading main meshes..." << std::endl;
	MeshBuffer const *ret = new MeshBuffer(data_path("proto-world.pnct"));
	main_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	std::cout << "loaded meshes" << std::endl;
	return ret;
});

Load< Scene > main_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("proto-world.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){\
		std::cout << "loading mesh: " << mesh_name << std::endl;
		Mesh const &mesh = main_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = main_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;
		std::cout << "loaded scene" << std::endl;
	});
});

Load< WalkMeshes > main_walkmeshes(LoadTagDefault, []() -> WalkMeshes const * {
	std::cout << "Loading walkmeshes..." << std::endl;
	WalkMeshes *ret = new WalkMeshes(data_path("proto-world.w"));
	std::cout << "loaded walkmeshes" << std::endl;
	return ret;
});

Load< Sound::Sample > music_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("PlaceHolder.opus"));
});

PlayMode::PlayMode() : scene(*main_scene) {
    //change depth buffer comparison function to be leq instead of less to correctly occlude in object detection
    glDepthFunc(GL_LEQUAL);

	// Find player mesh and transform
	for (auto& transform : scene.transforms) {
		if (transform.name == "Player") player.transform = &transform;
	}
	if (player.transform == nullptr) throw std::runtime_error("Player transform not found.");

	// Set up cameras
	{
		// Grab camera in scene for player's eyes
		if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, specifically at the player's head, but it has " + std::to_string(scene.cameras.size()));
		player.camera = &scene.cameras.back();
		player.camera->transform->parent = player.transform;

		// Create default PlayerCamera for taking pictures
		scene.transforms.emplace_back(); // Create new transform, will be parented to player.transform
		player.player_camera = std::make_unique<PlayerCamera>(&scene.transforms.back(), player.camera->transform, player.camera->fovy); // fovy being set to the same between both cameras 
	}

	// Setup walk mesh
	{
		player.walk_mesh = &main_walkmeshes->lookup("WalkMesh");
		// Start player walking at nearest walk point
		player.at = player.walk_mesh->nearest_walk_point(player.transform->position);
	}

	// Set up text renderer
	ui_text = new TextRenderer(data_path("LibreBarcode39-Regular.ttf"), ui_font_size);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
		else if (evt.button.button == SDL_BUTTON_LEFT) { // https://wiki.libsdl.org/SDL_Event and https://wiki.libsdl.org/SDL_MouseButtonEvent#:~:text=SDL_MouseButtonEvent%20is%20a%20member%20of,a%20button%20on%20a%20mouse.
			lmb.downs += 1;
			lmb.pressed = true;
			return true;
		}
		else if (evt.button.button == SDL_BUTTON_RIGHT) {
			rmb.downs += 1;
			rmb.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONUP) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			if (evt.button.button == SDL_BUTTON_LEFT) {
				lmb.pressed = false;
				return true;
			}
			else if (evt.button.button == SDL_BUTTON_RIGHT) {
				rmb.pressed = false;
				return true;
			}
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			mouse_motion = glm::vec2(evt.motion.xrel / float(window_size.y), -evt.motion.yrel / float(window_size.y));
			player.OnMouseMotion(mouse_motion);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	
	// Player movement
	{
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x = -1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y = -1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		if (move.x != 0.0f || move.y != 0.0f) player.Move(move, elapsed);
	}

	// Player camera logic 
	{
		// Toggle player view on right click
		if (rmb.downs == 1) {
			player.in_cam_view = !player.in_cam_view;
		}
		// Snap a pic on left click, if in camera view
		if (player.in_cam_view && lmb.downs == 1) {
			player.player_camera->TakePicture(scene, pictures);
		}
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	lmb.downs = 0;
	rmb.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

	// Update camera aspect ratios for drawable
	player.camera->aspect = float(drawable_size.x) / float(drawable_size.y);
	player.player_camera->scene_camera->aspect = float(drawable_size.x) / float(drawable_size.y);
    player.camera->drawable_size = drawable_size;
    player.player_camera->scene_camera->drawable_size = drawable_size;

	// Set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	// Draw scene based on active camera (Player's "eyes" or their PlayerCamera)
	if (player.in_cam_view) {
		scene.draw(*player.player_camera->scene_camera);
	}
	else {
		scene.draw(*player.camera);
	}

    //for debug, print all visible objects
//    {
//        std::list<std::pair<Scene::Drawable &, GLuint>> results;
//        if (player.in_cam_view) {
//            scene.render_picture(*player.player_camera->scene_camera, results);
//        } else {
//            scene.render_picture(*player.camera, results);
//        }
//        for (auto &guy: results) {
//            std::cout << guy.first.transform->name << std::endl;
//        }
//        std::cout << "\n" << std::endl;
//    }

	/* Debug code for visualizing walk mesh
	{
		glDisable(GL_DEPTH_TEST);
		DrawLines lines(player.camera->make_projection() * glm::mat4(player.camera->transform->make_world_to_local()));
		for (auto const &tri : player.walk_mesh->triangles) {
			lines.draw(player.walk_mesh->vertices[tri.x], player.walk_mesh->vertices[tri.y], glm::u8vec4(0x88, 0x00, 0x00, 0xff));
			lines.draw(player.walk_mesh->vertices[tri.y], player.walk_mesh->vertices[tri.z], glm::u8vec4(0x88, 0x00, 0x00, 0xff));
			lines.draw(player.walk_mesh->vertices[tri.z], player.walk_mesh->vertices[tri.x], glm::u8vec4(0x88, 0x00, 0x00, 0xff));
		}
	}
	*/

	// UI
	{
		if (player.in_cam_view) {
			ui_text->draw("place holder", 0.1f * float(drawable_size.x), 0.1f * float(drawable_size.y), 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
		}
	}
	GL_ERRORS();
}
