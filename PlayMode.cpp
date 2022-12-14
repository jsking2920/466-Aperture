#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "BoneLitColorTextureProgram.hpp"

#include "ShadowProgram.hpp"

#include "DrawLines.hpp"
#include "Sprite.hpp"
#include "DrawSprites.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"
#include "Framebuffers.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/detail/type_mat4x4.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <random>


// -------- Loading functions -----------
Load< std::map< std::string, CreatureStats > > creature_stats_map_load(LoadTagEarly, []() -> std::map< std::string, CreatureStats > const* {
    //Automatically parses Creature csv and puts results in Creature::creature_stats_map
    //TODO: make the stats a struct, not a vector of strings (low priority)
    Creature::creature_stats_map.clear();
    std::ifstream csv;
    csv.open(data_path("assets/ApertureNaming - CreatureSheet.csv"), std::ifstream::in);
    //verify that we can find this file
    if (!bool(csv)) {
        throw std::runtime_error("Could not find ApertureNaming - CreatureSheet.csv, ");
    }
    std::string buffer;
    getline(csv, buffer); //skip label line

    int index = 0;
    while (getline(csv, buffer)) {
        size_t delimiter_pos = 0;
        //get code
        delimiter_pos = buffer.find(',');
        const std::string code = buffer.substr(0, delimiter_pos);
        buffer.erase(0, delimiter_pos + 1);

        std::vector<std::string> row;
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
        //push back switch index
        row.push_back(std::to_string(index));
        CreatureStats temp(row);
        Creature::creature_stats_map.emplace(std::piecewise_construct, std::make_tuple(code), std::forward_as_tuple(row));
        index++;
    }
    return &Creature::creature_stats_map;
});

Load< SpriteAtlas > ui_sprites_atlas(LoadTagDefault, []() -> SpriteAtlas const* {
	return new SpriteAtlas(data_path("assets/sprites/ui_sprites")); // Each atlas needs to have a .atlas and .png file with this name at this path
});

GLuint main_meshes_for_lit_color_texture_program = 0;
GLuint main_meshes_for_depth_program = 0;
GLuint main_meshes_for_bone_lit_color_texture_program = 0;
Load< MeshBuffer > main_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("assets/proto-world2.pnct"));
	main_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
    main_meshes_for_depth_program = ret->make_vao_for_program(shadow_program->program);
	return ret;
});

GLuint FLO_banims_for_bone_lit_color_texture_program = 0;
GLuint FLO_banims_for_bone_shadow_program = 0;
Load< BoneAnimation > FLO_banims(LoadTagDefault, []() -> BoneAnimation const * {
    auto ret = new BoneAnimation(data_path("assets/animations/anim_FLO.banims"));
    BoneAnimation::animation_map.emplace(std::make_pair("FLO", ret));
    FLO_banims_for_bone_lit_color_texture_program = ret->make_vao_for_program(bone_lit_color_texture_program->program);
    FLO_banims_for_bone_shadow_program = ret->make_vao_for_program(bone_shadow_program->program);
    return ret;
});

GLuint MEP_banims_for_bone_lit_color_texture_program = 0;
GLuint MEP_banims_for_bone_shadow_program = 0;
Load< BoneAnimation > MEP_banims(LoadTagDefault, []() -> BoneAnimation const * {
    auto ret = new BoneAnimation(data_path("assets/animations/anim_MEP.banims"));
    BoneAnimation::animation_map.emplace(std::make_pair("MEP", ret));
    MEP_banims_for_bone_lit_color_texture_program = ret->make_vao_for_program(bone_lit_color_texture_program->program);
    MEP_banims_for_bone_shadow_program =  ret->make_vao_for_program(bone_shadow_program->program);
    return ret;
});

GLuint TAN_banims_for_bone_lit_color_texture_program = 0;
GLuint TAN_banims_for_bone_shadow_program = 0;
Load< BoneAnimation > TAN_banims(LoadTagDefault, []() -> BoneAnimation const * {
    auto ret = new BoneAnimation(data_path("assets/animations/anim_TAN.banims"));
    BoneAnimation::animation_map.emplace(std::make_pair("TAN", ret));
    TAN_banims_for_bone_lit_color_texture_program = ret->make_vao_for_program(bone_lit_color_texture_program->program);
    TAN_banims_for_bone_shadow_program =  ret->make_vao_for_program(bone_shadow_program->program);
    return ret;
});

GLuint TRI_banims_for_bone_lit_color_texture_program = 0;
GLuint TRI_banims_for_bone_shadow_program = 0;
Load< BoneAnimation > TRI_banims(LoadTagDefault, []() -> BoneAnimation const * {
    auto ret = new BoneAnimation(data_path("assets/animations/anim_TRI.banims"));
    BoneAnimation::animation_map.emplace(std::make_pair("TRI", ret));
    TRI_banims_for_bone_lit_color_texture_program = ret->make_vao_for_program(bone_lit_color_texture_program->program);
    TRI_banims_for_bone_shadow_program =  ret->make_vao_for_program(bone_shadow_program->program);
    return ret;
});

GLuint SNA_banims_for_bone_lit_color_texture_program = 0;
GLuint SNA_banims_for_bone_shadow_program = 0;
Load< BoneAnimation > SNA_banims(LoadTagDefault, []() -> BoneAnimation const * {
    auto ret = new BoneAnimation(data_path("assets/animations/anim_SNA.banims"));
    BoneAnimation::animation_map.emplace(std::make_pair("SNA", ret));
    SNA_banims_for_bone_lit_color_texture_program = ret->make_vao_for_program(bone_lit_color_texture_program->program);
    SNA_banims_for_bone_shadow_program =  ret->make_vao_for_program(bone_shadow_program->program);
    return ret;
});

GLuint PEN_banims_for_bone_lit_color_texture_program = 0;
GLuint PEN_banims_for_bone_shadow_program = 0;
Load< BoneAnimation > PEN_banims(LoadTagDefault, []() -> BoneAnimation const * {
    auto ret = new BoneAnimation(data_path("assets/animations/anim_PEN.banims"));
    BoneAnimation::animation_map.emplace(std::make_pair("PEN", ret));
    PEN_banims_for_bone_lit_color_texture_program = ret->make_vao_for_program(bone_lit_color_texture_program->program);
    PEN_banims_for_bone_shadow_program =  ret->make_vao_for_program(bone_shadow_program->program);
    return ret;
});

Load< Scene > main_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("assets/proto-world2.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const mesh_name, GLuint tex){
        Mesh const &mesh = main_meshes->lookup(mesh_name);
        //lock drawables
        std::unique_lock<std::mutex> lock(scene.drawable_load_mutex);
        scene.drawables.emplace_back(transform);
        Scene::Drawable &drawable = scene.drawables.back();
        //unlock drawables
        lock.unlock();

        //to find if creature (has anims)
        auto is_creature = [&](std::pair< std::string, CreatureStats > pair) {
            return pair.first == transform->name.substr(0, 3);
        };

		//TODO: for stuff that has animations, add a section where it samples the animation
        //only change shader if the object has a creature code
		if (transform->name.length() == 6 &&
                std::find_if(creature_stats_map_load->begin(), creature_stats_map_load->end(), is_creature) != creature_stats_map_load->end()) {
            int creature_index = creature_stats_map_load->at(transform->name.substr(0, 3)).switch_index;
            //animated object pipeline setup
			drawable.pipeline[Scene::Drawable::ProgramTypeDefault] = bone_lit_color_texture_program_pipeline;
            drawable.pipeline[Scene::Drawable::ProgramTypeDefault].type = mesh.type;
            //set roughnesses, possibly should be from csv??
            drawable.roughness = 0.9f;
            //this will get changed later
            drawable.pipeline[Scene::Drawable::ProgramTypeDefault].set_uniforms = [drawable](){
                glUniform1f(bone_lit_color_texture_program->ROUGHNESS_float, drawable.roughness);
            };

            //Set up depth program
            drawable.pipeline[Scene::Drawable::ProgramTypeShadow].program = bone_shadow_program_pipeline.program;
            drawable.pipeline[Scene::Drawable::ProgramTypeShadow].type = mesh.type;

            drawable.pipeline[Scene::Drawable::ProgramTypeShadow].OBJECT_TO_CLIP_mat4 = bone_shadow_program_pipeline.OBJECT_TO_CLIP_mat4;
            drawable.pipeline[Scene::Drawable::ProgramTypeShadow].OBJECT_TO_LIGHT_mat4x3 = bone_shadow_program_pipeline.OBJECT_TO_LIGHT_mat4x3;

            switch(creature_index) {
                case 0: {
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].start = FLO_banims->mesh.start;
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].count = FLO_banims->mesh.count;
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].vao = FLO_banims_for_bone_lit_color_texture_program;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].start = FLO_banims->mesh.start;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].count = FLO_banims->mesh.count;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].vao = FLO_banims_for_bone_shadow_program;
                    break;
                }
                case 1: {
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].vao = MEP_banims_for_bone_lit_color_texture_program;
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].start = MEP_banims->mesh.start;
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].count = MEP_banims->mesh.count;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].vao = MEP_banims_for_bone_shadow_program;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].start = MEP_banims->mesh.start;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].count = MEP_banims->mesh.count;
                    break;
                }
                case 2: {
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].vao = TAN_banims_for_bone_lit_color_texture_program;
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].start = TAN_banims->mesh.start;
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].count = TAN_banims->mesh.count;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].vao = TAN_banims_for_bone_shadow_program;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].start = TAN_banims->mesh.start;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].count = TAN_banims->mesh.count;
                    break;
                }
                case 3: {
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].vao = TRI_banims_for_bone_lit_color_texture_program;
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].start = TRI_banims->mesh.start;
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].count = TRI_banims->mesh.count;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].vao = TRI_banims_for_bone_shadow_program;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].start = TRI_banims->mesh.start;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].count = TRI_banims->mesh.count;
                    break;
                }
                case 4: {
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].vao = SNA_banims_for_bone_lit_color_texture_program;
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].start = SNA_banims->mesh.start;
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].count = SNA_banims->mesh.count;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].vao = SNA_banims_for_bone_shadow_program;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].start = SNA_banims->mesh.start;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].count = SNA_banims->mesh.count;
                    break;
                }
                case 5: {
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].vao = PEN_banims_for_bone_lit_color_texture_program;
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].start = PEN_banims->mesh.start;
                    drawable.pipeline[Scene::Drawable::ProgramTypeDefault].count = PEN_banims->mesh.count;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].vao = PEN_banims_for_bone_shadow_program;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].start = PEN_banims->mesh.start;
                    drawable.pipeline[Scene::Drawable::ProgramTypeShadow].count = PEN_banims->mesh.count;
                    break;
                }
                default: {
                    std::cout << "creature uniforms not set!" << std::endl;
                    break;
                }
            }
		} else {
            //Non-animated object
            drawable.pipeline[Scene::Drawable::ProgramTypeDefault] = lit_color_texture_program_pipeline;

            drawable.pipeline[Scene::Drawable::ProgramTypeDefault].vao = main_meshes_for_lit_color_texture_program;
            drawable.pipeline[Scene::Drawable::ProgramTypeDefault].type = mesh.type;
            drawable.pipeline[Scene::Drawable::ProgramTypeDefault].start = mesh.start;
            drawable.pipeline[Scene::Drawable::ProgramTypeDefault].count = mesh.count;

            //set roughnesses, possibly should be from csv??
            drawable.roughness = 0.9f;
            drawable.pipeline[Scene::Drawable::ProgramTypeDefault].set_uniforms = [drawable](){
                glUniform1f(lit_color_texture_program->ROUGHNESS_float, drawable.roughness);
            };

            //Set up depth program
            drawable.pipeline[Scene::Drawable::ProgramTypeShadow].program = shadow_program_pipeline.program;
            drawable.pipeline[Scene::Drawable::ProgramTypeShadow].vao = main_meshes_for_depth_program;
            drawable.pipeline[Scene::Drawable::ProgramTypeShadow].type = mesh.type;
            drawable.pipeline[Scene::Drawable::ProgramTypeShadow].start = mesh.start;
            drawable.pipeline[Scene::Drawable::ProgramTypeShadow].count = mesh.count;

            drawable.pipeline[Scene::Drawable::ProgramTypeShadow].OBJECT_TO_CLIP_mat4 = shadow_program_pipeline.OBJECT_TO_CLIP_mat4;
            drawable.pipeline[Scene::Drawable::ProgramTypeShadow].OBJECT_TO_LIGHT_mat4x3 = shadow_program_pipeline.OBJECT_TO_LIGHT_mat4x3;
        }

        // load texture for object if one exists, supports only 1 texture for now
		// texture must share name with transform in scene ( "assets/textures/{transform->name}.png" )
        // file existence check from https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exists-using-standard-c-c11-14-17-c
		std::string identifier = transform->name.substr(0, 6);
        if (std::filesystem::exists(data_path("assets/textures/" + identifier + ".png"))) {
            drawable.uses_vertex_color = false;
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
Load< std::unordered_map<std::string, Sound::Sample> > audio_samples(LoadTagDefault, []() -> std::unordered_map<std::string, Sound::Sample> const* {
    auto *sample_map = new std::unordered_map<std::string, Sound::Sample>();
    std::vector<std::string> names = {
            //insert new samples here
            "CameraClick",
            "FLO_Bounce",
            "FLO_Idle",
            "Footstep",
            "Page_Turn",
            "Strange_New_World.L",
            "Strange_New_World.R",
            "TRI_Idle",
            "TAN_Roar",
            "SNA_Hide",
            "PEN_Idle",
            "PEN_Idle_2",
            "PEN_Angry",
            "MEP_Idle"
    };
    for(std::string name : names) {
        sample_map->emplace(std::piecewise_construct, std::make_tuple(name), std::make_tuple(data_path("assets/audio/" + name + ".opus")));
    }
    return sample_map;
});


//* -------- Mode initializationand cleanup ---------- */
PlayMode::PlayMode() : scene(*main_scene) {
	
    // Change depth buffer comparison function to be leq instead of less to correctly occlude in object detection
    glDepthFunc(GL_LEQUAL);

	Scene::Transform *player_transform = nullptr;
	
	// Find player mesh and transform
	for (auto& transform : scene.transforms) {
		if (transform.name == "Player") player_transform = &transform;
	}
	if (player_transform == nullptr) throw std::runtime_error("Player transform not found.");
	
	// Set up player and camera
	{
		// Check for camera in scene for player's eyes
		if (scene.cameras.size() != 2) {
			std::cout << "Expecting scene to have exactly two cameras, specifically at the player's head and one overhead for the menu, but it has " + std::to_string(scene.cameras.size()) << std::endl;
		}
		Scene::Camera* player_cam = nullptr;

		for (auto c = scene.cameras.begin(); c != scene.cameras.end(); c++) {
			if (c->transform->name == "overhead_camera") {
				overhead_cam = &(*c);
			}
			else if (c->transform->name == "player_camera") {
				player_cam = &(*c);
			}
		}
		if (player_cam == nullptr || overhead_cam == nullptr) {
			std::runtime_error("Missing either 'overhead_camera' or 'player_cam' in the scene");
		}

		// Create new transform for Player's PlayerCamera
		scene.transforms.emplace_back();

		player = new Player(player_transform, &main_walkmeshes->lookup("WalkMe"), player_cam, &scene.transforms.back());
		cur_spawn = player->at;
	}
	
	// Set up text renderers
	display_text = new TextRenderer(data_path("assets/fonts/Audiowide-Regular.ttf"), display_font_size);
	handwriting_text = new TextRenderer(data_path("assets/fonts/PoorStory-Regular.ttf"), handwriting_font_size);
	body_text = new TextRenderer(data_path("assets/fonts/Sono-Regular.ttf"), body_font_size);
	barcode_text = new TextRenderer(data_path("assets/fonts/LibreBarcode128Text-Regular.ttf"), barcode_font_size);

    //load audio samples
    std::cout<<"loading audio..."<<std::endl;
    sample_map = *audio_samples;
    Sound::sample_map = &sample_map; // example access--> Sound::play(Sound::sample_map->at("CameraClick"));

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
                }
                //    std::cout << "found primary focal point:" << trans.name << std::endl;
                //} else {
                //    std::cout << "found extra focal point:" << trans.name << std::endl;
                //}
                creature.focal_points.push_back(&draw);
                draw.render_to_screen = false;
                draw.render_to_picture = false;
            }
        }
    }

	//animation initialization
	{
		for (auto &creature_pair : Creature::creature_map) {
			Creature &critter = creature_pair.second;
            critter.play_animation("Idle", true, 1.0f);
            critter.animation_player->position = (float)rand() / (float)RAND_MAX;
		}
	}

	// Set up menu screen
	{
		active_camera = overhead_cam;
		// Hide player
		player->transform->position -= glm::vec3(0.0f, 0.0f, 10.0f);
	}

    //set up original positions
    {
        for(auto &pair : Creature::creature_map) {
            pair.second.original_pos = pair.second.transform->position;
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
        else if (evt.key.keysym.sym == SDLK_LSHIFT) {
            lshift.downs += 1;
            lshift.pressed = true;
        }
		else if (evt.key.keysym.sym == SDLK_r) {
			r.downs += 1;
			r.pressed = true;
		}
		else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right_arrow.downs += 1;
			right_arrow.pressed = true;
		}
		else if (evt.key.keysym.sym == SDLK_LEFT) {
			left_arrow.downs += 1;
			left_arrow.pressed = true;
		}
		else if (evt.key.keysym.sym == SDLK_BACKSPACE) {
			backspace.downs += 1;
			backspace.pressed = true;
		}
		else if (evt.key.keysym.sym == SDLK_DELETE) {
			del.downs += 1;
			del.pressed = true;
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
        else if (evt.key.keysym.sym == SDLK_LSHIFT) {
            lshift.pressed = false;
        }
		else if (evt.key.keysym.sym == SDLK_r) {
			r.pressed = false;
		}
		else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right_arrow.pressed = false;
		}
		else if (evt.key.keysym.sym == SDLK_LEFT) {
			left_arrow.pressed = false;
		}
		else if (evt.key.keysym.sym == SDLK_BACKSPACE) {
			backspace.pressed = false;
		}
		else if (evt.key.keysym.sym == SDLK_DELETE) {
			del.pressed = false;
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
			// Can be multiple of these events per frame, so sum them all up
			mouse.mouse_motion += glm::vec2(evt.motion.xrel / float(window_size.y), -evt.motion.yrel / float(window_size.y));
			mouse.moves += 1;
			return true;
		}
	} else if (evt.type == SDL_MOUSEWHEEL) {
		if (evt.wheel.y != 0) // scroll mouse wheel
		{
			mouse.scrolled = true;
			mouse.wheel_y += evt.wheel.y; // Can be multiple of these events per frame, so sum them all up
		}
        if (evt.wheel.x != 0)
        {
            mouse.scrolled = true;
            mouse.wheel_x += evt.wheel.x; // Can be multiple of these events per frame, so sum them all up
        }
	}
	return false;
}

void PlayMode::update(float elapsed) {

	time_of_day += elapsed * time_scale * time_scale_debug;

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
	r.downs = 0;
	right_arrow.downs = 0;
	left_arrow.downs = 0;
	del.downs = 0;
	backspace.downs = 0;

	mouse.moves = 0;
	mouse.mouse_motion = glm::vec2(0, 0);
	mouse.wheel_x = 0;
	mouse.wheel_y = 0;
	mouse.scrolled = false;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

	// Update camera aspect ratios for drawable
	{
		active_camera->aspect = float(drawable_size.x) / float(drawable_size.y);
		active_camera->drawable_size = drawable_size;

        // Based on: https://github.com/15-466/15-466-f20-framebuffer
        // Make sure framebuffers are the same size as the window:
        framebuffers.realloc(drawable_size, glm::vec2(1024, 1024));
	}

	// Handle scene lighting, forward lighting based on https://github.com/15-466/15-466-f19-base6/blob/master/DemoLightingForwardMode.cpp
	{
        glm::vec3 eye = active_camera->transform->make_local_to_world()[3];

        // Compute light uniforms:
        GLsizei lights = uint32_t(scene.lights.size() + 2);
        // Clamp lights to maximum lights allowed by shader:
        lights = std::min< uint32_t >(lights, LitColorTextureProgram::MaxLights);

        // Lighting information vectors
        std::vector< int32_t > light_type; light_type.reserve(lights);
        std::vector< glm::vec3 > light_location; light_location.reserve(lights);
        std::vector< glm::vec3 > light_direction; light_direction.reserve(lights);
        std::vector< glm::vec3 > light_energy; light_energy.reserve(lights);
        std::vector< float > light_cutoff; light_cutoff.reserve(lights);

        // Hemisphere light is 1
        light_type.emplace_back(1);
        light_cutoff.emplace_back(1.0f);
        // Sun/Moon is 2
        light_type.emplace_back(3);
        light_cutoff.emplace_back(1.0f);

		// Calculate brightness of sun/moon based in time of day
        // Fix "jump" at day/night switch over, by letting brightnesses reach zero and turning up ambient lighting
        // Maybe displace sunset and sunrise to be more during the daytime so that sun angles make more sense during sunrise/set
        // make sunrise less orange probably
        glm::vec3 ambient_color;
		float brightness;
        glm::vec3 sun_angle;
		glm::vec3 sun_color;

		// daytime lighting
		if (time_of_day >= sunrise && time_of_day <= sunset) { 
			// sinusoidal curve that goes from 0 at sun rise to 1 at the midpoint between sun rise and set to 0 at sun set
			brightness = std::sin(((time_of_day - sunrise) / (sunset - sunrise)) * float(M_PI));
			// increase amplitude of curve so that middle of the day is all at or above max brightness
			brightness *= 1.3f;
            float color_sin = std::clamp(brightness, 0.0f, 1.0f);

            sun_color = glm::vec3(1.0f, 1.0f, 0.95f); // slightly warm light (less blue)
            sun_angle = glm::vec3(0, -std::cos(((time_of_day - sunrise) / (sunset - sunrise)) * float(M_PI)) / std::sqrt(2),
                                   -std::sin(((time_of_day - sunrise) / (sunset - sunrise)) * float(M_PI)) / std::sqrt(2));
            sky_color = day_sky_color * color_sin + sunset_sky_color * std::pow( 1 - color_sin, 3.f );
            ambient_color = day_ambient_color * color_sin + sunset_ambient_color * std::pow( 1 - color_sin, 3.f );
		}
		// nighttime lighting
		else { 
			float unwrapped_time = time_of_day < sunset ? day_length + time_of_day : time_of_day; // handle timer wrapping aorund to 0
			// sinusoidal curve that goes from 0 at sun set to 1 at the midpoint between sun set and rise to 0 at sun rise
			float sin = std::sin(((unwrapped_time - sunset) / (sunrise + (day_length - sunset))) * float(M_PI));

			brightness = (sin * 0.8f);
            sun_color = glm::vec3(0.975f, 0.975f, 1.0f); // slightly cool light (more blue) that is also dimmer
            sun_angle = glm::vec3(0, -std::cos(((unwrapped_time - sunset) / (sunrise + (day_length - sunset))) * float(M_PI)) / std::sqrt(2),
                                     -std::sin(((unwrapped_time - sunset) / (sunrise + (day_length - sunset))) * float(M_PI)) / std::sqrt(2));
            sky_color = night_sky_color * sin + sunset_sky_color * std::pow( 1 - sin, 3.f );
            ambient_color = night_ambient_color * sin + sunset_ambient_color * std::pow( 1 - sin, 2.f );
		}
        sun_color *= brightness;

        fog_intensity = 0.6f + 0.4f * (1 - brightness);
        fog_color = glm::vec3(0.4f) + 0.6f * sky_color;

        // Push calculated sky lighting uniforms
        light_direction.emplace_back(glm::vec3(0, 0, -1.f));
        light_energy.emplace_back(ambient_color);
        light_location.emplace_back(glm::vec3(0, 0, 100));
        // Push calculated sun lighting uniforms
        light_direction.emplace_back(sun_angle);
        light_energy.emplace_back(sun_color);
        light_location.emplace_back(player->transform->position - sun_angle * 50.f);

        // Other lights setup
        for (auto const &light : scene.lights) {
            if (light.type == Scene::Light::Hemisphere || light.type == Scene::Light::Directional) {
				std::cout << "Only one hemisphere & directional light is allowed! Not adding this one!" << std::endl;
                continue;
            }

            glm::mat4 light_to_world = light.transform->make_local_to_world();
            // Set up lighting information for this light:
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

            // Skip remaining lights if maximum light count reached:
			if (light_type.size() == (uint32_t)lights) {
				std::cout << "Max light count reached!" << std::endl;
				break;
			}
        }

        // Set up sky lighting uniforms for lit_color_texture_program:
        glUseProgram(lit_color_texture_program->program);

        glUniform3fv(lit_color_texture_program->EYE_vec3, 1, glm::value_ptr(eye));
        glUniform1ui(lit_color_texture_program->LIGHTS_uint, (GLuint) lights);
        glUniform1iv(lit_color_texture_program->LIGHT_TYPE_int_array, lights, light_type.data());
        glUniform3fv(lit_color_texture_program->LIGHT_LOCATION_vec3_array, lights, glm::value_ptr(light_location[0]));
        glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3_array, lights, glm::value_ptr(light_direction[0]));
        glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3_array, lights, glm::value_ptr(light_energy[0]));
        glUniform1fv(lit_color_texture_program->LIGHT_CUTOFF_float_array, lights, light_cutoff.data());

        // Bone textures
        glUseProgram(bone_lit_color_texture_program->program);
        glUniform3fv(bone_lit_color_texture_program->EYE_vec3, 1, glm::value_ptr(eye));
        glUniform1ui(bone_lit_color_texture_program->LIGHTS_uint, (GLuint) lights);
        glUniform1iv(bone_lit_color_texture_program->LIGHT_TYPE_int_array, lights, light_type.data());
        glUniform3fv(bone_lit_color_texture_program->LIGHT_LOCATION_vec3_array, lights, glm::value_ptr(light_location[0]));
        glUniform3fv(bone_lit_color_texture_program->LIGHT_DIRECTION_vec3_array, lights, glm::value_ptr(light_direction[0]));
        glUniform3fv(bone_lit_color_texture_program->LIGHT_ENERGY_vec3_array, lights, glm::value_ptr(light_energy[0]));
        glUniform1fv(bone_lit_color_texture_program->LIGHT_CUTOFF_float_array, lights, light_cutoff.data());

        GL_ERRORS();

        //Draw scene to shadow map for spotlight, adapted from https://github.com/ixchow/15-466-f18-base3
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers.shadow_fb);
        glViewport(0, 0, framebuffers.shadow_size.x, framebuffers.shadow_size.y);

        glClearColor(1.0f, 0.0f, 1.0f, 0.0f); //clear color for shadow buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        //render only back faces to shadow map (prevent shadow speckles on fronts of objects):
        glCullFace(GL_FRONT);
        glEnable(GL_CULL_FACE);

        //get sun "position" to be the sun's angle away from the player in order to use it to create transformation matrix
        //from https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
        glm::vec3 sun_pos = player->transform->position - glm::normalize(sun_angle) * 100.f;
        glm::mat4 lightView = glm::lookAt(sun_pos, player->transform->position, glm::vec3(0.f, 1.f, 0.f));//TODO: set sun distance correctly, check if correct firection & probably change angle
        glm::mat4 orthographic_projection = glm::ortho(-20.f, 20.f, -20.f, 20.f, 0.01f, 500.f);
        glm::mat4 const world_to_clip = orthographic_projection * lightView;

        glm::mat4 world_to_spot =
                //This matrix converts from the spotlight's clip space ([-1,1]^3) into depth map texture coordinates ([0,1]^2) and depth map Z values ([0,1]):
                glm::mat4(
                        0.5f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.5f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.5f, 0.0f,
                        0.5f, 0.5f, 0.5f+0.00001f /* <-- bias */, 1.0f
                )
                //this is the world-to-clip matrix used when rendering the shadow map:
                * world_to_clip;
        glUseProgram(lit_color_texture_program->program);
        glUniformMatrix4fv(lit_color_texture_program->LIGHT_TO_SPOT_mat4, 1, GL_FALSE, glm::value_ptr(world_to_spot));

        glUseProgram(bone_lit_color_texture_program->program);
        glUniformMatrix4fv(bone_lit_color_texture_program->LIGHT_TO_SPOT_mat4, 1, GL_FALSE, glm::value_ptr(world_to_spot));
        glUseProgram(0);

        scene.draw(Scene::Drawable::PassTypeShadow, world_to_clip, glm::mat4x3(1.0f));

        glDisable(GL_CULL_FACE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        GL_ERRORS(); //now texture is already in framebuffers.shadow_depth_tex
	}

    // Run depth pre-pass for occlusion query, and write position buffer for post-processing
    {
        // run query for each drawable
        glViewport(0, 0, drawable_size.x, drawable_size.y);
        // bind renderbuffers for rendering
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers.oc_fb);

        // set clear depth, testing criteria, and the like
        glClearDepth(1.0f); // 1.0 is the default value to clear the depth buffer to, but you can change it
        //set sky bits to be far away from the camera
        //glm::vec4 distant = active_camera->transform->make_local_to_world() * glm::vec4(active_camera->transform->position, 1.0f) + glm::vec4(1000.f, 1000.f, 1000.f, 0.0f);
        //glclearcolor(distant.r, distant.g, distant.b, distant.a);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clears currently bound framebuffer's color and depth info to clearColor set above (0, 0 ,0 1) and clearDepth set above (1.0)
        glEnable(GL_DEPTH_TEST); // enable depth testing
        glDepthFunc(GL_LEQUAL); // set criteria for depth test
        glDisable(GL_BLEND);
        GL_ERRORS();

        // render with occlusion pass
        scene.draw(*active_camera, Scene::Drawable::PassTypePrepass);
    }

    // Run occlusion query using prepass sample buffer
//    {
//        glDepthMask(GL_FALSE); // depth buffer should be already filled
//        GL_ERRORS();
//        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
//
//        // Render with occlusion pass
//        scene.draw(*active_camera, Scene::Drawable::PassTypeOcclusion);
//
//        // Reenable writing
//        glDepthMask(GL_TRUE);
//        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//        glBindFramebuffer(GL_FRAMEBUFFER, 0);
//
//        glUseProgram(0);
//        glBindVertexArray(0);
//
//        GL_ERRORS();
//    }
	
	// Draw scene to multisampled framebuffer
	{
		// Based on: https://github.com/15-466/15-466-f20-framebuffer

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffers.ms_fb);
        glViewport(0, 0, drawable_size.x, drawable_size.y);
        // Set "sky" (clear color)
        glClearColor(sky_color.x, sky_color.y, sky_color.z, 1.0f);

        //bind generated shadow texture
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, framebuffers.shadow_depth_tex);

		// set clear depth, testing criteria, and the like
		glClearDepth(1.0f); // 1.0 is the default value to clear the depth buffer to, but you can change it
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clears currently bound framebuffer's (framebuffers.ms_fb) color and depth info
		                                                    // clears color to clearColor set above (sky_color) and clearDepth set above (1.0)
		glEnable(GL_DEPTH_TEST); // enable depth testing
		glDepthFunc(GL_LEQUAL); // set criteria for depth test
        glDepthMask(GL_TRUE);

		// TODO: implement blending, currently doesn't work because objects are being rendered in arbitrary order (maybe?)
		// glEnable(GL_BLEND);
		// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Draw based on active camera (Player's "eyes" or their PlayerCamera)
        if (player->in_cam_view) {
            scene.draw(*active_camera, Scene::Drawable::PassTypeInCamera);
        } else {
            scene.draw(*active_camera, Scene::Drawable::PassTypeDefault);
        }

        // Unbind textures
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, 0);
	}

	// Debugging code for printing all visible objects and visualizing walk mesh
	/*
	{
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

		{
			glDisable(GL_DEPTH_TEST);
			DrawLines lines(player->camera->make_projection() * glm::mat4(player->camera->transform->make_world_to_local()));
			for (auto const &tri : player->walk_mesh->triangles) {
				lines.draw(player->walk_mesh->vertices[tri.x], player->walk_mesh->vertices[tri.y], glm::u8vec4(0x88, 0x00, 0x00, 0xff));
				lines.draw(player->walk_mesh->vertices[tri.y], player->walk_mesh->vertices[tri.z], glm::u8vec4(0x88, 0x00, 0x00, 0xff));
				lines.draw(player->walk_mesh->vertices[tri.z], player->walk_mesh->vertices[tri.x], glm::u8vec4(0x88, 0x00, 0x00, 0xff));
			}
		}
	}
	*/

    // Resolve multisampled buffer to screen
    {
        // blit multisampled buffer to the normal, intermediate post_processing buffer. Image is stored in screen_texture, depth in pp_depth
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffers.ms_fb);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffers.pp_fb);

        glBlitFramebuffer(0, 0, drawable_size.x, drawable_size.y, 0, 0, drawable_size.x, drawable_size.y, GL_COLOR_BUFFER_BIT, GL_LINEAR); // Bilinear interpolation for anti aliasing

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        GL_ERRORS();
    }

	// Postprocessing
    { 
        // Add fog, fog uses original multisampled depth buffer so no aliasing
        framebuffers.add_depth_effects(fog_intensity, 1300.0f, fog_color);

        if (player->in_cam_view) {
            //add depth of field
            framebuffers.add_depth_of_field(player->player_camera->cur_focus, active_camera->transform->make_local_to_world()[3]);
            // Copy framebuffer to main window:
            framebuffers.tone_map_to_screen(framebuffers.screen_texture);
        } else {
            framebuffers.tone_map_to_screen(framebuffers.depth_effect_tex);
        }
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
	if (enter.downs > 0) {
		time_of_day = start_day_time;
		time_scale = TIME_SCALE_DEFAULT;
		active_camera = player->camera;

		// reset player position, unhiding them
		player->transform->position = player->walk_mesh->to_world_point(player->at);

        music_l = Sound::play(Sound::sample_map->at("Strange_New_World.L"), MUSIC_VOLUME, 1.0f, -1.0f);
        music_r = Sound::play(Sound::sample_map->at("Strange_New_World.R"), MUSIC_VOLUME, 1.0f, 1.0f);

		cur_state = playing;
		return;
	}

	// Give cam a little back and forth sway (this math is more what you would call "artistic" than "scientific")
	overhead_cam_timer += elapsed / 4.0f;
	overhead_cam_timer = overhead_cam_timer >= float(M_PI) * 2 ? 0.0f : overhead_cam_timer; // timer loops over [0, 2pi]
	float angle = glm::sin(overhead_cam_timer) * 0.05f;

	overhead_cam->transform->rotation = glm::angleAxis(glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)) * overhead_cam->transform->rotation;
}

void PlayMode::menu_draw_ui(glm::uvec2 const& drawable_size) {

	display_text->draw("APERTURE", 0.3f * float(drawable_size.x), 0.5f * float(drawable_size.y), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
	body_text->draw("press enter to start", 0.35f * float(drawable_size.x), 0.4f * float(drawable_size.y), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
}

// -------- Playing functions -----------
void PlayMode::playing_update(float elapsed) {

	// Handle State (return early if state changes)
	{
		// open journal on tab, swap to journal state
		
		if (tab.downs > 0) {
            Sound::play(Sound::sample_map->at("Page_Turn"));
            if(music_l && music_r) {
                music_l->set_paused(true, 2.0f / 60.f);
                music_r->set_paused(true, 2.0f / 60.f);
            }
			player->in_cam_view = false;
			player->SetCrouch(false);

			time_scale = 0.0f; // time doesn't stop while in journal but it could

			cur_journal_pg = 0; // Always open journal to first page

			cur_state = journal;
			return;
		}


		// swap to night state at end of day
		if (time_of_day >= end_day_time && time_of_day < start_day_time) {

			player->in_cam_view = false;
			player->SetCrouch(false);

			time_scale = 0.0f; // pause at night to review pics
			cur_pic_to_review = 0;
			started_reviewing_pics = false;

			active_camera = overhead_cam;

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

		if (move.x != 0.0f || move.y != 0.0f) {
            player->Move(move, elapsed);
            //SFX
            time_since_last_footstep += elapsed;
            if(time_since_last_footstep > footstep_time/player->get_speed()) {
                float random = ((float) rand() / (RAND_MAX)); //from https://stackoverflow.com/questions/9878965/rand-between-0-and-1
                Sound::play(Sound::sample_map->at("Footstep"), ((float)(rand() % 2) + 4)/10, random/4 + 0.875f ); //pitch and volume randomization
                time_since_last_footstep = 0;
            }
        }
		if (player->is_crouched != lctrl.pressed) player->SetCrouch(lctrl.pressed);
		if (player->is_running != lshift.pressed) player->SetRun(lshift.pressed);
	}

	// Player camera logic 
	{
		// First person looking around on mouse movement
		if (mouse.moves > 0) {
			player->OnMouseMotion(mouse.mouse_motion * mouse_sensitivity);
		}

		// Toggle player view on right click
		if (rmb.downs > 0) {
			if (player->in_cam_view) {
				active_camera = player->camera;
			}
			else {
				active_camera = player->player_camera->scene_camera;
			}
			player->in_cam_view = !player->in_cam_view;
		}

		if (player->in_cam_view && mouse.scrolled) {
            // Change Depth of Field distance on shift + mouse scroll 
            if (lshift.pressed) {
                //on mac, shift + scroll is x
                if (mouse.wheel_x > 0 || mouse.wheel_y > 0) {
                    player->player_camera->AdjustFocus(true); // Increase focal distance and depth of field
                } else {
                    player->player_camera->AdjustFocus(false); // Decrease focal distance and depth of field
                }
            } else {
                // Zoom in and out on mouse wheel scroll (without shift) when in cam view (also affects focal distance)
                if (mouse.wheel_y > 0) {
                    player->player_camera->AdjustZoom(true); // zoom in
                } else {
                    player->player_camera->AdjustZoom(false); // zoom out
                }
            }
		}

		if (r.downs > 0) {
			// Reset camera zoom and focus on r
			player->player_camera->Reset(false);
		}

		// Snap a pic on left click, if in camera view and has remaining battery
		if (player->in_cam_view && lmb.downs == 1 && player->player_camera->cur_battery > 0) {
			player->player_camera->TakePicture(scene);
		}
	}

    //creature movement updates
    std::for_each(Creature::creature_map.begin(), Creature::creature_map.end(), [&](std::pair< const std::string, Creature > &pair) {
        Creature &creature = pair.second;
        creature.update(elapsed, time_of_day, player->transform->make_local_to_world()[3]);
    });
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
		display_text->draw("x" + std::to_string(zoom / 10) + "." + std::to_string(zoom % 10), ((2.0f / 3.0f) - 0.04f) * float(drawable_size.x), ((1.0f / 3.0f) - 0.05f) * float(drawable_size.y), 0.25f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));

		// Focal distance readout
		// Should be some combination of aperture diameter and lens length, but we'll just abstract it to something resembling aperrture diameter
		// Larger depth of field means smaller aperture diameter, but is typically displayed in terms of a fraction (f/4 = 1/4 which is larger than f/16)
		// Techincally this implies exposure of the picture should be affected too, which it's not
		// see: https://photographylife.com/what-is-aperture-in-photography
		// Ranges from f/1 to f/30, which is wider than in the real world but it's ~sci-fi~
		// Quantized to discrete values in much wider increments than actual increments available to player 
		// Remap backend represenation of focal distance to range for ddisplay (1 to 30)
		float t = (player->player_camera->cur_focus - player->player_camera->min_focus) / (player->player_camera->max_focus - player->player_camera->min_focus);
		float denom = 1.0f + (t * (30.0f - 1.0f));
		uint8_t denom_rounded = (uint8_t)denom;
		display_text->draw("f/" + std::to_string(denom_rounded), 0.9f * float(drawable_size.x), 0.9f * float(drawable_size.y), 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));

		// Battery readout
		float battery = (float)player->player_camera->cur_battery / (float)player->player_camera->max_battery;
		display_text->draw("Battery: " + TextRenderer::format_percentage(battery), 0.05f * float(drawable_size.x), 0.025f * float(drawable_size.y), 0.4f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
		
		// Creature in frame text
		// TODO: implement this feature (need to check for creatures each frame)
		barcode_text->draw("APERTURE", (1.0f / 3.0f) * float(drawable_size.x), ((1.0f / 3.0f) - 0.05f) * float(drawable_size.y), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
		
		// Some assorted DSLR type viewport readouts
		// AUTO in palce of any Exposure/ISO settings

		// Just some numbers for the vibes, could be a date, time, shutter speed, etc.
		uint16_t time_to_display = (uint16_t)abs(time_of_day - 10.0f);
		uint8_t day_to_display = (9 + day) % 30;
		uint8_t month_to_display = 2; // TODO: implement months
		display_text->draw("[" + std::to_string(month_to_display) + "." + std::to_string(day_to_display) + "." + std::to_string(time_to_display) + "]", 0.85f * float(drawable_size.x), 0.025f * float(drawable_size.y), 0.4f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
	}
	else {
		// Draw clock
        //moved back an hour
		body_text->draw(TextRenderer::format_time_of_day(fmod(time_of_day + day_length - 10.0f, day_length), day_length), 0.025f * float(drawable_size.x), 0.025f * float(drawable_size.y), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
	}
}

// -------- Journal functions -----------
void PlayMode::journal_update(float elapsed) {
	// swap to night state at end of day, even if in journal
	if (time_of_day >= end_day_time && time_of_day < start_day_time) {
		time_scale = 0.0f; // pause at night to review pics
		cur_pic_to_review = 0;
		started_reviewing_pics = false;
		active_camera = overhead_cam;
		cur_state = night;
		return;
	}

	// close journal on tab, swap to playing state
	if (tab.downs == 1) {
		time_scale = TIME_SCALE_DEFAULT; // if time freezes in journal, would need to start it moving again
        if(music_l && music_r) {
            music_l->set_paused(false, 0.5f, MUSIC_VOLUME);
            music_r->set_paused(false, 0.5f, MUSIC_VOLUME);
        }
		cur_journal_pg = 0;
		cur_state = playing;
		return;
	}
	// Page left on a or left arrow
	else if (left.downs > 0 || left_arrow.downs > 0) {
		//page left
	}
	// Page right on d or right arrow
	else if (right.downs > 0 || right_arrow.downs > 0) {
		// page right
	}
}

void PlayMode::journal_draw_ui(glm::uvec2 const& drawable_size) {

	// Draw blank book background for journal
	DrawSprites draw(*ui_sprites_atlas, glm::vec2(0, 0), glm::vec2(1920, 1080), drawable_size);
	draw.draw(ui_sprites_atlas->lookup("journal_bg"), glm::vec2(0.62f * float(drawable_size.x), 0.65f * float(drawable_size.y)), 1.0f);
	draw.flush_sprite_buffer();

	// Page title
	handwriting_text->draw("CREATURES", 0.2f * float(drawable_size.x), 0.85f * float(drawable_size.y), 1.0f, journal_text_color, float(drawable_size.x), float(drawable_size.y));

	// Draw every picture saved by the player
	float offset = 0.85f / 4.f + 0.04f;
	int i = 1;
    int j = 0;
    float xoffset = 0.36f;



//	for (auto p = saved_pictures.begin(); p != saved_pictures.end(); ++p) {
//
//
//		// title
//		handwriting_text->draw((*p)->title, 0.15f * float(drawable_size.x), (0.8f - (offset * i)) * float(drawable_size.y), 1.0f, journal_text_color, float(drawable_size.x), float(drawable_size.y));
//		// actual picture
//		DrawPicture pic(**p, drawable_size);
//		// scale is relative to entire screen resolution, so 1 means full screen coverage
//		pic.draw(glm::vec2(0.5f * float(drawable_size.x), (0.8f - (offset * i)) * float(drawable_size.y)), 0.1f);
//		i++;
//	}

    uint32_t total_score = 0;

	for (auto c = Creature::creature_stats_map.begin(); c != Creature::creature_stats_map.end(); ++c) {
		// Creature name
        float x = (0.15f + xoffset * j) * float(drawable_size.x);
        float y = (0.98f - (offset * i)) * float(drawable_size.y);
        if(c->second.is_discovered()) {
            total_score += c->second.best_picture->get_total_score();
            handwriting_text->draw((*c).second.name, x,
                                    y, 1.0f, journal_text_color,
                                   float(drawable_size.x), float(drawable_size.y));
            handwriting_text->draw("Times Photographed: " + std::to_string(c->second.times_photographed), x,
                                   y - 0.05f * float(drawable_size.y), 0.5f, journal_text_color,
                                   float(drawable_size.x), float(drawable_size.y));
            handwriting_text->draw("Best score: " + std::to_string(c->second.best_picture->get_total_score()), x,
                                   y - 0.1f * float(drawable_size.y), 0.5f, journal_text_color,
                                   float(drawable_size.x), float(drawable_size.y));

            //draw pic
		    DrawPicture pic(*c->second.best_picture, drawable_size);
            pic.draw(glm::vec2(x + 0.233f * float(drawable_size.x), y - (offset / 4) * float(drawable_size.y)), 0.16f);

        } else {
            handwriting_text->draw("?????", x,
                                   y, 1.0f, journal_text_color,
                                   float(drawable_size.x), float(drawable_size.y));
        }
        i++;
        if(i > 3) {
            j++;
            i = 1;
        }
	}

    handwriting_text->draw("Total score: " + std::to_string(total_score), (0.2f + 0.34f) * float(drawable_size.x), 0.85f * float(drawable_size.y), 1.0f, journal_text_color, float(drawable_size.x), float(drawable_size.y));


}

// -------- Nightime functions -----------
void PlayMode::night_update(float elapsed) {


	/*
		As pictures taken through the day, they accumulate in player.pictures
		If a pic is taken that is the best of a given creature, it is store in that creatures struct
		At the end of the day, player sees all pictures taken and gets a chance to save them
		 - saving them puts them in scrapbook in journal, and saves them to hard drive
		player.pictures is cleared at end of day
	*/

	if (!started_reviewing_pics) {
		if (enter.downs > 0) {
			started_reviewing_pics = true;
			finished_reviewing_pics = player->pictures.size() == 0;
		}
	}
	else {
		if (cur_pic_to_review < player->pictures.size()) {
			// TODO: make these buttons
			// save pic being reviewed on enter
			if (enter.downs > 0) {
				saved_pictures.push_back(player->pictures[cur_pic_to_review]);
				player->pictures[cur_pic_to_review]->save_picture_png();
				cur_pic_to_review++;
			}
			// skip pic being reviewed on delete or backspace
			else if (del.downs > 0 || backspace.downs > 0) {
				cur_pic_to_review++;
			}
		}

		if (cur_pic_to_review >= player->pictures.size()) {
			// Clear out unsaved pics and zoom through the night
			finished_reviewing_pics = true;
			player->ClearPictures();

			// swap to playing at start of day
			// reset player position
			player->at = cur_spawn;
			player->transform->position = player->walk_mesh->to_world_point(player->at);

			// Reset camera zoom, focus, and battery
			player->player_camera->Reset(true);

			time_of_day = start_day_time;
			time_scale = TIME_SCALE_DEFAULT;
			day++;

			active_camera = player->camera;

            for(auto &pair : Creature::creature_map) {
                pair.second.transform->position = pair.second.original_pos;
            }

            for(auto &pair : Creature::creature_map) {
                pair.second.reset();
            }

            music_l = Sound::play(Sound::sample_map->at("Strange_New_World.L"), MUSIC_VOLUME, 1.0f, -1.0f);
            music_r = Sound::play(Sound::sample_map->at("Strange_New_World.R"), MUSIC_VOLUME, 1.0f, 1.0f);


            cur_state = playing;
		}
	}

	// Give cam a little back and forth sway (this math is more what you would call "artistic" than "scientific")
	overhead_cam_timer += elapsed / 4.0f;
	overhead_cam_timer = overhead_cam_timer >= float(M_PI) * 2 ? 0.0f : overhead_cam_timer; // timer loops over [0, 2pi]
	float angle = glm::sin(overhead_cam_timer) * 0.05f;

	overhead_cam->transform->rotation = glm::angleAxis(glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)) * overhead_cam->transform->rotation;
}

void PlayMode::night_draw_ui(glm::uvec2 const& drawable_size) {

	// Draw clock
	// body_text->draw(TextRenderer::format_time_of_day(time_of_day, day_length), 0.025f * float(drawable_size.x), 0.025f * float(drawable_size.y), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));

	if (!started_reviewing_pics) {
		display_text->draw("End of Day", 0.31f * float(drawable_size.x), 0.5f * float(drawable_size.y), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));

		if (player->pictures.size() > 0) {
			body_text->draw("(press ENTER to review your photos)", 0.36f * float(drawable_size.x), 0.05f * float(drawable_size.y), 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
		}
		else {
			body_text->draw("(press ENTER to sleep)", 0.41f * float(drawable_size.x), 0.05f * float(drawable_size.y), 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
		}
	}
	else if (!finished_reviewing_pics) {
		Picture &p = *player->pictures[cur_pic_to_review].get();

		// Draw pic being reviewed with some score info and the like
		DrawPicture pic(p, drawable_size);
		pic.draw(glm::vec2(0.315f * float(drawable_size.x), 0.6f * float(drawable_size.y)), 0.6f);

		// Picture Title
		display_text->draw(p.title, 0.635f * float(drawable_size.x), 0.85f * float(drawable_size.y), 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
		// Scoring elements
		std::list<std::string> scoring_elements = p.get_scoring_strings();
		float offset = 0.0f;
		float spacing = float(drawable_size.y) * 0.035f;
		for (auto s = scoring_elements.begin(); s != scoring_elements.end(); s++) {
			body_text->draw(*s, 0.635f * float(drawable_size.x), (0.75f * float(drawable_size.y)) - offset, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
			offset += spacing;
		}

		// Instructions for saving/skipping a pic
		body_text->draw("(press ENTER to save picture or DELETE to skip it)", 0.31f * float(drawable_size.x), 0.05f * float(drawable_size.y), 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), float(drawable_size.x), float(drawable_size.y));
	}
}
