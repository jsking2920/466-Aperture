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
#include <glm/detail/type_mat4x4.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>

#include <random>

GLuint main_meshes_for_lit_color_texture_program = 0;
GLuint main_meshes_for_lit_color_program = 0;
Load< MeshBuffer > main_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("assets/proto-world2.pnct"));
	main_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > main_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("assets/proto-world2.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
        Mesh const &mesh = main_meshes->lookup(mesh_name);
        scene.drawables.emplace_back(transform);
        Scene::Drawable &drawable = scene.drawables.back();

        drawable.pipeline = lit_color_texture_program_pipeline;

        drawable.pipeline.vao = main_meshes_for_lit_color_texture_program;
        drawable.pipeline.type = mesh.type;
        drawable.pipeline.start = mesh.start;
        drawable.pipeline.count = mesh.count;

        //set roughnesses, possibly should be from csv??
        float roughness = 1.0f;
		//if (transform->name.substr(0, 9) == "Icosphere") {
			//roughness = (transform->position.y + 10.0f) / 18.0f;
		//}
        drawable.pipeline.set_uniforms = [drawable, roughness]() {
			glUniform1f(lit_color_texture_program->ROUGHNESS_float, roughness);
		};

        GLuint tex;
        glGenTextures(1, &tex);

        // load texture for object if one exists, supports only 1 texture for now
		// texture must share name with transform in scene ( "assets/textures/{transform->name}.png" )
        // file existence check from https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exists-using-standard-c-c11-14-17-c

		std::string identifier = transform->name.substr(0, 6);
        if (std::filesystem::exists(data_path("assets/textures/" + identifier + ".png"))) {
            drawable.uses_vertex_color = false;

            glBindTexture(GL_TEXTURE_2D, tex);
            glm::uvec2 size;
            std::vector< glm::u8vec4 > tex_data;

            load_png(data_path("assets/textures/" + identifier + ".png"), &size, &tex_data, LowerLeftOrigin);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
			glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // change this to GL_REPEAT or something else for texture tiling
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
         
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // mipmapping for textures far from cam
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Bilinear filtering for textures close to cam
            glBindTexture(GL_TEXTURE_2D, 0);

            drawable.pipeline.textures[0].texture = tex;
            drawable.pipeline.textures[0].target = GL_TEXTURE_2D;

            GL_ERRORS(); // check for errros
        } else {
            //no texture found, using vertex colors
            drawable.uses_vertex_color = true;
        }
	});
});

Load< WalkMeshes > main_walkmeshes(LoadTagDefault, []() -> WalkMeshes const * {
	WalkMeshes *ret = new WalkMeshes(data_path("assets/proto-world2.w"));
	return ret;
});

//Automatically loads samples with names listed in names vector. names vector can include paths
Load< std::map<std::string, Sound::Sample> > audio_samples(LoadTagDefault, []() -> std::map<std::string, Sound::Sample> const* {
    auto *sample_map = new std::map<std::string, Sound::Sample>();
    std::vector<std::string> names = {
            //insert new samples here
            "CameraClick",
    };
    for(std::string name : names) {
        sample_map->emplace(std::piecewise_construct, std::make_tuple(name), std::make_tuple(data_path("assets/audio/" + name + ".opus")));
    }
    return sample_map;
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

		player = new Player(player_transform, &main_walkmeshes->lookup("WalkMe"), &scene.cameras.back(), &scene.transforms.back());
		cur_spawn = player->at;
	}
	
	// Set up text renderers
	display_text = new TextRenderer(data_path("assets/fonts/Audiowide-Regular.ttf"), display_font_size);
	body_text = new TextRenderer(data_path("assets/fonts/Audiowide-Regular.ttf"), body_font_size);
	barcode_text = new TextRenderer(data_path("assets/fonts/LibreBarcode128Text-Regular.ttf"), barcode_font_size);

    //load audio samples
    sample_map = *audio_samples;
    Sound::sample_map = &sample_map; // example access--> Sound::play(Sound::sample_map->at("CameraClick"));

    //Automatically parses Creature csv and puts results in Creature::creature_stats_map
    //TODO: make the stats a struct, not a vector of strings (low priority)
    {
        Creature::creature_stats_map.clear();
        std::ifstream csv ("assets/ApertureNaming - CreatureSheet.csv", std::ifstream::in);
        std::string buffer;
        getline(csv, buffer); //skip label line

        while (getline(csv, buffer)) {
            size_t delimiter_pos = 0;
            //get code
            delimiter_pos = buffer.find(',');
            std::string code = buffer.substr(0, delimiter_pos);
            buffer.erase(0, delimiter_pos + 1);

            Creature::creature_stats_map.emplace(std::piecewise_construct, make_tuple(code), std::make_tuple());
            std::vector<std::string> &row = Creature::creature_stats_map[code];
            //row.reserve(buffer.length);
            row.push_back(code);

            //loop through comma delimited columns
            //from https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
            while ((delimiter_pos = buffer.find(',')) != std::string::npos) {
                row.push_back(buffer.substr(0, delimiter_pos));
                buffer.erase(0, delimiter_pos + 1);
            }
            //run once for last column, not delimited by comma
            row.push_back(buffer.substr(0, delimiter_pos));
            buffer.erase(0, delimiter_pos + 1);
        }
    }

    // using syntax from https://stackoverflow.com/questions/14075128/mapemplace-with-a-custom-value-type
    // if we use things that need references in the future, change make_tuple to forward_as_tuple
    // put in constructor??
    for (Scene::Drawable &draw : scene.drawables) {
        Scene::Transform &trans = *draw.transform;
        std::string &name = trans.name;
        std::string id_code = name.substr(0, 6);
        //if stats exist but creature has not been built yet, initialize creature
        if (Creature::creature_stats_map.count(name.substr(0,3)) && !Creature::creature_map.count(id_code)) {
            Creature::creature_map.emplace(std::piecewise_construct, 
				                           std::make_tuple(id_code), 
				                           std::make_tuple(name.substr(0, 3),
										   std::stoi(name.substr(4, 2))));
        }
        //if creature already exists, set up
        if (Creature::creature_map.count(id_code)) {
            Creature &creature = Creature::creature_map[id_code];
            if (trans.name == id_code) {
                creature.transform = &trans;
                creature.drawable = &draw;
            }

            if (trans.name.length() >= 10 && trans.name.substr(7, 3) == "foc") {
                if (trans.name.substr(7, 6) == "foc_00") {
                    creature.focal_point = &trans;
                    std::cout << "found primary focal point:" << trans.name << std::endl;
                } else {
                    std::cout << "found extra focal point:" << trans.name << std::endl;
                }
                creature.focal_points.push_back(&draw);
                draw.render_to_screen = false;
                draw.render_to_picture = false;
            }
        }
    }
}

PlayMode::~PlayMode() {
	delete player;
    Sound::sample_map = nullptr;
}

// -------- Main functions -----------
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
		else if (evt.key.keysym.sym == SDLK_TAB) {
			tab.downs += 1;
			tab.pressed = true;
		}
		else if (evt.key.keysym.sym == SDLK_RETURN) {
			enter.downs += 1;
			enter.pressed = true;
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
		else if (evt.key.keysym.sym == SDLK_TAB) {
			tab.pressed = false;
		}
		else if (evt.key.keysym.sym == SDLK_RETURN) {
			enter.pressed = false;
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
			mouse.mouse_motion = glm::vec2(evt.motion.xrel / float(window_size.y), -evt.motion.yrel / float(window_size.y));
			mouse.moved = true;
			return true;
		}
	} else if (evt.type == SDL_MOUSEWHEEL && player->in_cam_view) {
		if (evt.wheel.y != 0) // scroll mouse wheel
		{
			mouse.scrolled = true;
			mouse.wheel_y = evt.wheel.y;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	time_of_day += elapsed * time_scale;

	switch (cur_state) {

		case menu:
			menu_update(elapsed);
			break;
		case playing:
			playing_update(elapsed);
			break;
		case journal:
			journal_update(elapsed);
			break;
		case night:
			night_update(elapsed);
			break;
	}

	// Loop day timer
	if (time_of_day > day_length) {
		time_of_day = 0.0f;
	}

	// Reset button press counters and mouse
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	lmb.downs = 0;
	rmb.downs = 0;
	lctrl.downs = 0;
	tab.downs = 0;
	enter.downs = 0;

	mouse.moved = false;
	mouse.scrolled = false;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

	// Update camera aspect ratios for drawable
	{
		player->camera->aspect = float(drawable_size.x) / float(drawable_size.y);
		player->player_camera->scene_camera->aspect = float(drawable_size.x) / float(drawable_size.y);
		player->camera->drawable_size = drawable_size;
		player->player_camera->scene_camera->drawable_size = drawable_size;
	}

    //set active camera
    Scene::Camera *active_camera;
    if (player->in_cam_view) {
        active_camera = player->player_camera->scene_camera;
    }
    else {
        active_camera = player->camera;
    }

	// Handle scene lighting, forward lighting based on https://github.com/15-466/15-466-f19-base6/blob/master/DemoLightingForwardMode.cpp
	{
        glm::vec3 eye = active_camera->transform->make_local_to_world()[3];

        //compute light uniforms:
        GLsizei lights = uint32_t(scene.lights.size() + 2);
        //clamp lights to maximum lights allowed by shader:
        lights = std::min< uint32_t >(lights, LitColorTextureProgram::MaxLights);

        //lighting information vectors
        std::vector< int32_t > light_type; light_type.reserve(lights);
        std::vector< glm::vec3 > light_location; light_location.reserve(lights);
        std::vector< glm::vec3 > light_direction; light_direction.reserve(lights);
        std::vector< glm::vec3 > light_energy; light_energy.reserve(lights);
        std::vector< float > light_cutoff; light_cutoff.reserve(lights);

        //hemisphere light is 1
        light_type.emplace_back(1);
        light_cutoff.emplace_back(1.0f);
        light_location.emplace_back(glm::vec3(0, 0, 100));
        //sun/moon is 2
        light_type.emplace_back(3);
        light_cutoff.emplace_back(1.0f);
        light_location.emplace_back(glm::vec3(0, 0, 100));


		// calculate brightness of sun/moon based in time of day
        //Fix "jump" at day/night switch over, by letting brightnesses reach zero and turning up ambient lighting
        //Maybe displace sunset and sunrise to be more during the daytime so that sun angles make more sense during sunrise/set
        //make sunrise less orange probably
        glm::vec3 sky_color;
        glm::vec3 ambient_color;
		float brightness;
        glm::vec3 sun_angle;
		glm::vec3 sun_color;
		if (time_of_day >= sunrise && time_of_day <= sunset) { // daytime lighting
			// sinusoidal curve that goes from 0 at sun rise to 1 at the midpoint between sun rise and set to 0 at sun set
			brightness = std::sin(((time_of_day - sunrise) / (sunset - sunrise)) * float(M_PI));
			// increase amplitude of curve so that middle of the day is all at or above max brightness
			brightness *= 1.3f;
            float color_sin = std::clamp(brightness, 0.0f, 1.0f);
			// lerp brightness value from 0.15 to 1.5 so that it's never completely dark
			//brightness = ((1.0f - brightness) * 0.15f) + brightness;
			// clamp brightness to [0.1f, 1.0f], which creates a "plateau" in the curve at midday
			brightness = brightness > 1.0f ? 1.0f : brightness;

            sun_color = glm::vec3(1.0f, 1.0f, 0.95f); // slightly warm light (less blue)
            sun_angle = glm::vec3(0, -std::cos(((time_of_day - sunrise) / (sunset - sunrise)) * float(M_PI)) / std::sqrt(2),
                                   -std::sin(((time_of_day - sunrise) / (sunset - sunrise)) * float(M_PI)) / std::sqrt(2));
            sky_color = day_sky_color * color_sin + sunset_sky_color * std::pow( 1 - color_sin, 3.f );
            ambient_color = day_ambient_color * color_sin + sunset_ambient_color * std::pow( 1 - color_sin, 3.f );
		}
		else { // nighttime lighting
			float unwrapped_time = time_of_day < sunset ? day_length + time_of_day : time_of_day; // handle timer wrapping aorund to 0
			// sinusoidal curve that goes from 0 at sun set to 1 at the midpoint between sun set and rise to 0 at sun rise
			float sin = std::sin(((unwrapped_time - sunset) / (sunrise + (day_length - sunset))) * float(M_PI));
			// lerp brightness value from 0.15 to 0.4 so that it's never completely dark but also never quite as bright as day
			brightness = (sin * 0.8f);
            sun_color = glm::vec3(0.975f, 0.975f, 1.0f); // slightly cool light (more blue) that is also dimmer
            sun_angle = glm::vec3(0, -std::cos(((unwrapped_time - sunset) / (sunrise + (day_length - sunset))) * float(M_PI)) / std::sqrt(2),
                                     -std::sin(((unwrapped_time - sunset) / (sunrise + (day_length - sunset))) * float(M_PI)) / std::sqrt(2));
            sky_color = night_sky_color * sin + sunset_sky_color * std::pow( 1 - sin, 3.f );
            ambient_color = night_ambient_color * sin + sunset_ambient_color * std::pow( 1 - sin, 2.f );
		}
        sun_color *= brightness;

        //push calculated sky lighting uniforms
        light_direction.emplace_back(glm::vec3(0, 0, -1.f));
        light_energy.emplace_back(ambient_color);
        //push calculated sun lighting uniforms
        light_direction.emplace_back(sun_angle);
        light_energy.emplace_back(sun_color);

        //other lights setup
        for (auto const &light : scene.lights) {
            //only one hemisphere & directional light is allowed! otherwise they won't get added
            if (light.type == Scene::Light::Hemisphere || light.type == Scene::Light::Directional) {
                continue;
            }

            glm::mat4 light_to_world = light.transform->make_local_to_world();
            //set up lighting information for this light:
            light_location.emplace_back(glm::vec3(light_to_world[3]));
            light_direction.emplace_back(glm::vec3(-light_to_world[2]));
            light_energy.emplace_back(light.energy);

            if (light.type == Scene::Light::Point) {
                light_type.emplace_back(0);
                light_cutoff.emplace_back(1.0f);
            } else if (light.type == Scene::Light::Spot) {
                light_type.emplace_back(2);
                light_cutoff.emplace_back(std::cos(0.5f * light.spot_fov));
            }

            //skip remaining lights if maximum light count reached:
            if (light_type.size() == (uint32_t)lights) break;
        }

        GL_ERRORS();


        // Set up sky lighting uniforms for lit_color_texture_program:
        glUseProgram(lit_color_texture_program->program);

        //getting warnings about narrowing conversions, if doesn't compile on windows this is probably why
        //to solve, cast all the locations to GLint
        glUniform3fv(lit_color_texture_program->EYE_vec3, 1, glm::value_ptr(eye));

        glUniform1ui(lit_color_texture_program->LIGHTS_uint, (GLuint) lights);

        glUniform1iv(lit_color_texture_program->LIGHT_TYPE_int_array, lights, light_type.data());
        glUniform3fv(lit_color_texture_program->LIGHT_LOCATION_vec3_array, lights, glm::value_ptr(light_location[0]));
        glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3_array, lights, glm::value_ptr(light_direction[0]));
        glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3_array, lights, glm::value_ptr(light_energy[0]));
        glUniform1fv(lit_color_texture_program->LIGHT_CUTOFF_float_array, lights, light_cutoff.data());
        glUseProgram(0);

		// Set "sky" (clear color)
		glClearColor(sky_color.x, sky_color.y, sky_color.z, 1.0f);
	}
	
	// Draw scene to multisampled framebuffer
	{
		// Based on: https://github.com/15-466/15-466-f20-framebuffer
		// Make sure framebuffers are the same size as the window:
		framebuffers.realloc(drawable_size);

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffers.ms_fb);

		// set clear depth, testing criteria, and the like
		glClearDepth(1.0f); // 1.0 is the default value to clear the depth buffer to, but you can change it
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clears currently bound framebuffer's (framebuffers.ms_fb )color and depth info
		                                                    // clears color to clearColor set above (sky_color) and clearDepth set above (1.0)
		glEnable(GL_DEPTH_TEST); // enable depth testing
		glDepthFunc(GL_LEQUAL); // set criteria for depth test

		// TODO: implement blending, currently doesn't work because objects are being rendered in arbitrary order (maybe?)
		// glEnable(GL_BLEND);
		// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Draw based on active camera (Player's "eyes" or their PlayerCamera)
		scene.draw(*active_camera);
	}

	// Debugging code
	{
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
	}

	// Resolve mulstisampled buffer to screen and perform post processing
	{
		// blit multisampled buffer to the normal, intermediate post_processing buffer. Image is stored in screen_texture
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffers.ms_fb);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffers.pp_fb);

		glBlitFramebuffer(0, 0, drawable_size.x, drawable_size.y, 0, 0, drawable_size.x, drawable_size.y, GL_COLOR_BUFFER_BIT, GL_LINEAR); // Bilinear interpolation for anti aliasing

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Copy framebuffer to main window:
		framebuffers.tone_map();
	}
	
	// Draw UI
	{
		switch (cur_state) {
			case menu:
				menu_draw_ui(drawable_size);
				break;
			case playing:
				playing_draw_ui(drawable_size);
				break;
			case journal:
				journal_draw_ui(drawable_size);
				break;
			case night:
				night_draw_ui(drawable_size);
				break;
		}
	}
	GL_ERRORS();
}

// -------- Menu functions -----------
void PlayMode::menu_update(float elapsed) {

	// start game on enter, swap to playing state
	if (enter.downs == 1) {
		time_of_day = start_day_time;
		time_scale = 1.0f;
		cur_state = playing;
		return;
	}
}

void PlayMode::menu_draw_ui(glm::uvec2 const& drawable_size) {

	display_text->draw("APERTURE", 0.4f * float(drawable_size.x), 0.5f * float(drawable_size.y), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
	body_text->draw("press enter to start", 0.35f * float(drawable_size.x), 0.4f * float(drawable_size.y), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
}

// -------- Playing functions -----------
void PlayMode::playing_update(float elapsed) {

	// Handle State (return early if state changes)
	{
		// open journal on tab, swap to journal state
		if (tab.downs == 1) {
			
			player->in_cam_view = false;
			player->SetCrouch(false);
			score_text_is_showing = false;

			time_scale = 1.0f; // time doesn't stop while in journal but it could

			cur_state = journal;
			return;
		}

		// swap to night state at end of day
		if (time_of_day == end_day_time) {

			player->in_cam_view = false;
			player->SetCrouch(false);
			score_text_is_showing = false;

			time_scale = 8.0f; // zoom through night

			cur_state = night;
			return;
		}
	}
	
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
			player->SetCrouch(lctrl.pressed);
		}
	}

	// Player camera logic 
	{
		// First person looking around on mouse movement
		if (mouse.moved) {
			player->OnMouseMotion(mouse.mouse_motion);
		}

		// Toggle player view on right click
		if (rmb.downs == 1) {
			player->in_cam_view = !player->in_cam_view;
		}

		// Zoom in and out on mouse wheel scroll when in cam view
		if (player->in_cam_view && mouse.scrolled) {
			if (mouse.wheel_y > 0) {
				player->player_camera->AdjustZoom(0.1f); // zoom in
			}
			else {
				player->player_camera->AdjustZoom(-0.1f); // zoom out
			}
		}

		// Snap a pic on left click, if in camera view and has remaining battery
		if (player->in_cam_view && lmb.downs == 1 && player->player_camera->cur_battery > 0) {
			player->player_camera->TakePicture(scene);
			score_text_is_showing = true;
			score_text_popup_timer = 0.0f;
		}
	}

	// UI
	{
		if (score_text_is_showing) {
			if (score_text_popup_timer >= score_text_popup_duration) {
				score_text_is_showing = false;
			}
			else {
				score_text_popup_timer += elapsed;
			}
		}
	}
}

void PlayMode::playing_draw_ui(glm::uvec2 const& drawable_size) {

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
		body_text->draw("x" + std::to_string(zoom / 10) + "." + std::to_string(zoom % 10), ((2.0f / 3.0f) - 0.04f) * float(drawable_size.x), ((1.0f / 3.0f) - 0.05f) * float(drawable_size.y), 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));

		// Battery readout
		float battery = (float)player->player_camera->cur_battery / (float)player->player_camera->max_battery;
		body_text->draw("Battery: " + TextRenderer::format_percentage(battery), 0.025f * float(drawable_size.x), 0.925f * float(drawable_size.y), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));

		// Creature in frame text
		// TODO: implement this feature (need to check for creatures each frame)
		barcode_text->draw("FLOATER", (1.0f / 3.0f) * float(drawable_size.x), ((1.0f / 3.0f) - 0.05f) * float(drawable_size.y), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
	}
	else {
		// Draw clock
		body_text->draw(TextRenderer::format_time_of_day(time_of_day, day_length), 0.025f * float(drawable_size.x), 0.025f * float(drawable_size.y), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
	}

	if (score_text_is_showing) {
		// draw text of last picture taken
		if (!player->pictures->empty()) {
			body_text->draw(player->pictures->back().title, 0.025f * float(drawable_size.x), 0.95f * float(drawable_size.y), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
			body_text->draw("Score: " + std::to_string(player->pictures->back().get_total_score()), 0.025f * float(drawable_size.x), 0.9f * float(drawable_size.y), 0.8f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
		}
	}
}

// -------- Journal functions -----------
void PlayMode::journal_update(float elapsed) {

	// swap to night state at end of day, even if in journal
	if (time_of_day == end_day_time) {
		time_scale = 8.0f; // zoom through night
		cur_state = night;
		return;
	}

	// close journal on tab, swap to playing state
	if (tab.downs == 1) {
		time_scale = 1.0f; // if time freezes in journal, would need to start it moving again
		cur_state = playing;
		return;
	}
}

void PlayMode::journal_draw_ui(glm::uvec2 const& drawable_size) {

	body_text->draw("JOURNAL", 0.45f * float(drawable_size.x), 0.85f * float(drawable_size.y), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
	// Draw clock
	body_text->draw(TextRenderer::format_time_of_day(time_of_day, day_length), 0.025f * float(drawable_size.x), 0.025f * float(drawable_size.y), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
}

// -------- Nightime functions -----------
void PlayMode::night_update(float elapsed) {

	// swap to playing at start of day
	if (time_of_day >= start_day_time) {
		//TODO: reset player position/etc
		player->player_camera->cur_battery = player->player_camera->max_battery;

		time_scale = 1.0f;
		cur_state = playing;
		return;
	}
}

void PlayMode::night_draw_ui(glm::uvec2 const& drawable_size) {

	// Draw clock
	body_text->draw(TextRenderer::format_time_of_day(time_of_day, day_length), 0.025f * float(drawable_size.x), 0.025f * float(drawable_size.y), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
}