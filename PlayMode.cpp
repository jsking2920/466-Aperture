#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"
#include "Framebuffers.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <filesystem>

#include <random>

GLuint main_meshes_for_lit_color_texture_program = 0;
GLuint main_meshes_for_lit_color_program = 0;
Load< MeshBuffer > main_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("assets/proto-world.pnct"));
	main_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > main_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("assets/proto-world.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
        Mesh const &mesh = main_meshes->lookup(mesh_name);
        scene.drawables.emplace_back(transform);
        Scene::Drawable &drawable = scene.drawables.back();

        drawable.pipeline = lit_color_texture_program_pipeline;

        drawable.pipeline.vao = main_meshes_for_lit_color_texture_program;
        drawable.pipeline.type = mesh.type;
        drawable.pipeline.start = mesh.start;
        drawable.pipeline.count = mesh.count;

        GLuint tex;
        glGenTextures(1, &tex);

        // load texture, supports only 1 texture for now
        // file existence check from https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exists-using-standard-c-c11-14-17-c
        if(std::filesystem::exists(data_path("assets/textures/" + transform->name + ".png"))) {
            drawable.uses_vertex_color = false;
            glBindTexture(GL_TEXTURE_2D, tex);
            glm::uvec2 size;
            std::vector< glm::u8vec4 > tex_data;
            load_png(data_path("assets/textures/" + transform->name + ".png"), &size, &tex_data, LowerLeftOrigin);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);

            drawable.pipeline.textures[0].texture = tex;
            drawable.pipeline.textures[0].target = GL_TEXTURE_2D;

            GL_ERRORS();
        } else {
            //no texture found, using vertex colors
            drawable.uses_vertex_color = true;
        }

	});
});

Load< WalkMeshes > main_walkmeshes(LoadTagDefault, []() -> WalkMeshes const * {
	WalkMeshes *ret = new WalkMeshes(data_path("assets/proto-world.w"));
	return ret;
});

Load< Sound::Sample > music_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("assets/audio/PlaceHolder.opus"));
});

PlayMode::PlayMode() : scene(*main_scene) {
	
    // Change depth buffer comparison function to be leq instead of less to correctly occlude in object detection
    glDepthFunc(GL_LEQUAL);

	Scene::Transform *player_transform = nullptr;
	
	// Find player mesh and transform
	for (auto& transform : scene.transforms) {
		if (transform.name == "Player") player_transform = &transform;
	}
	if (player_transform == nullptr) throw std::runtime_error("Player transform not found.");
	
	// Set up player
	{
		// Check for camera in scene for player's eyes
		if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, specifically at the player's head, but it has " + std::to_string(scene.cameras.size()));
		
		// Create new transform for Player's PlayerCamera
		scene.transforms.emplace_back();

		player = new Player(player_transform, &main_walkmeshes->lookup("WalkMesh"), &scene.cameras.back(), &scene.transforms.back());
	}
	
	// Set up text renderer
	display_text = new TextRenderer(data_path("assets/fonts/Audiowide-Regular.ttf"), display_font_size);
	barcode_text = new TextRenderer(data_path("assets/fonts/LibreBarcode128Text-Regular.ttf"), barcode_font_size);


	// Load creatures, should eventually loop through codes and/or models
    //  using syntax from https://stackoverflow.com/questions/14075128/mapemplace-with-a-custom-value-type
    //  if we use things that need references in the future, change make_tuple to forward_as_tuple
    //  put in constructor??
    std::string id_code = "FLO0";
    Creature::creature_map.emplace(std::piecewise_construct, std::make_tuple(id_code), std::make_tuple("Floater", "FLO", 0, 0));
    Creature &creature = Creature::creature_map[id_code];
    creature.init_transforms(scene);
}

PlayMode::~PlayMode() {
	delete player;
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
		else if (evt.key.keysym.sym == SDLK_LCTRL) {
			lctrl.downs += 1;
			lctrl.pressed = true;
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
		else if (evt.key.keysym.sym == SDLK_LCTRL) {
			lctrl.pressed = false;
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
			// Look around on mouse motion
			mouse_motion = glm::vec2(evt.motion.xrel / float(window_size.y), -evt.motion.yrel / float(window_size.y));
			player->OnMouseMotion(mouse_motion);
			return true;
		}
	} else if (evt.type == SDL_MOUSEWHEEL && player->in_cam_view) {
		if (evt.wheel.y > 0) // scroll up
		{
			player->player_camera->AdjustZoom(0.1f); // zoom in
		}
		else if (evt.wheel.y < 0) // scroll down
		{
			player->player_camera->AdjustZoom(-0.1f); // zoom out
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	
	// Player movement
	{
		// WASD to move on walk mesh
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x = -1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y = -1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		if (move.x != 0.0f || move.y != 0.0f) player->Move(move, elapsed);

		// Hold lctrl to crouch
		if (lctrl.pressed != player->is_crouched) {
			player->ToggleCrouch();
		}
	}
	
	// Player camera logic 
	{
		// Toggle player view on right click
		if (rmb.downs == 1) {
			player->in_cam_view = !player->in_cam_view;
		}
		// Snap a pic on left click, if in camera view
		if (player->in_cam_view && lmb.downs == 1) {
			player->player_camera->TakePicture(scene);
		}
	}
	
	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	lmb.downs = 0;
	rmb.downs = 0;
	lctrl.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

    // Code block from https://github.com/15-466/15-466-f20-framebuffer
    // Make sure framebuffers are the same size as the window:
    framebuffers.realloc(drawable_size);
	
	// Update camera aspect ratios for drawable
	player->camera->aspect = float(drawable_size.x) / float(drawable_size.y);
	player->player_camera->scene_camera->aspect = float(drawable_size.x) / float(drawable_size.y);
    player->camera->drawable_size = drawable_size;
    player->player_camera->scene_camera->drawable_size = drawable_size;

	// Set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

    glUseProgram(lit_color_texture_program->program);
    glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
    glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
    glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
    glUseProgram(0);

    // Draw to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers.hdr_fb);
	
	glClearColor(0.5f, 1.0f, 1.0f, 1.0f);
	glClearDepth(1.0f); // 1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	// Draw scene based on active camera (Player's "eyes" or their PlayerCamera)
	if (player->in_cam_view) {
		scene.draw(*player->player_camera->scene_camera);
	}
	else {
		scene.draw(*player->camera);
	}

    /* Debug code for printint all visible objects
    {
		std::list<std::pair<Scene::Drawable &, GLuint>> results;
        if (player.in_cam_view) {
			scene.render_picture(*player.player_camera->scene_camera, results);
        } else {
			scene.render_picture(*player.camera, results);
        }
        for (auto &guy: results) {
			std::cout << guy.first.transform->name << std::endl;
        }
        std::cout << "\n" << std::endl;
    }
	*/

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

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // Copy framebuffer to main window:
    framebuffers.tone_map();
	
	// UI
	{
		if (player->in_cam_view) {
			// Draw viewport grid
			DrawLines grid(glm::mat4(
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			));

			// Viewport rule of thirds guidelines
			grid.draw(glm::vec3(-1.0f / 3.0f, 1.0f / 3.0f, 0), glm::vec3(1.0f / 3.0f, 1.0f / 3.0f, 0), glm::u8vec4(0xff));
			grid.draw(glm::vec3(-1.0f / 3.0f, -1.0f / 3.0f, 0), glm::vec3(1.0f / 3.0f, -1.0f / 3.0f, 0), glm::u8vec4(0xff));
			grid.draw(glm::vec3(-1.0f / 3.0f, 1.0f / 3.0f, 0), glm::vec3(-1.0f / 3.0f, -1.0f / 3.0f, 0), glm::u8vec4(0xff));
			grid.draw(glm::vec3(1.0f / 3.0f, 1.0f / 3.0f, 0), glm::vec3(1.0f / 3.0f, -1.0f / 3.0f, 0), glm::u8vec4(0xff));
			// Viewport reticle (square if aspect ratio is 16x9)
			grid.draw(glm::vec3(-1.0f / 16.0f, 1.0f / 9.0f, 0), glm::vec3(1.0f / 16.0f, 1.0f / 9.0f, 0), glm::u8vec4(0xff));
			grid.draw(glm::vec3(-1.0f / 16.0f, -1.0f / 9.0f, 0), glm::vec3(1.0f / 16.0f, -1.0f / 9.0f, 0), glm::u8vec4(0xff));
			grid.draw(glm::vec3(-1.0f / 16.0f, 1.0f / 9.0f, 0), glm::vec3(-1.0f / 16.0f, -1.0f / 9.0f, 0), glm::u8vec4(0xff));
			grid.draw(glm::vec3(1.0f / 16.0f, 1.0f / 9.0f, 0), glm::vec3(1.0f / 16.0f, -1.0f / 9.0f, 0), glm::u8vec4(0xff));

			// Zoom level readout
			uint8_t zoom = (uint8_t)(std::round(player->player_camera->cur_zoom * 10.0f));
			display_text->draw("x" + std::to_string(zoom / 10) + "." + std::to_string(zoom % 10), ((2.0f / 3.0f) - 0.04f) * float(drawable_size.x), ((1.0f / 3.0f) - 0.05f) * float(drawable_size.y), 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));

			// Creature in frame text
			// TODO: implement this feature (need to check for creatures each frame
			barcode_text->draw("FLOATER", (1.0f / 3.0f) * float(drawable_size.x), ((1.0f / 3.0f) - 0.05f) * float(drawable_size.y), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
		}
		else {
			// draw text of last picture taken
			if (!player->pictures->empty()) {
				display_text->draw(player->pictures->back().title, 0.025f * float(drawable_size.x), 0.95f * float(drawable_size.y), 0.6f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
				display_text->draw("Score: " + std::to_string(player->pictures->back().get_total_score()), 0.025f * float(drawable_size.x), 0.9f * float(drawable_size.y), 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
			}
		}
	}
	GL_ERRORS();
}
