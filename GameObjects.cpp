#include "GameObjects.hpp"
#include "Scene.hpp"
#include "glm/gtx/string_cast.hpp"
#include "BoneLitColorTextureProgram.hpp"
#include "ShadowProgram.hpp"
#include "Sound.hpp"

//#define _USE_MATH_DEFINES << gives redefine warning ??
//#define M_2_PI     0.636619772367581343076
#include <math.h>

std::map< std::string, Creature > Creature::creature_map = std::map< std::string, Creature >();
std::map< std::string, CreatureStats > Creature::creature_stats_map = std::map< std::string, CreatureStats >();


CreatureStats::CreatureStats(std::vector< std::string >& strings)
{
    assert(strings.size() == 6); //if assert fails, you probably added a field to the csv that must be added here!
    code = strings[0];
    name = strings[1];
    description = strings[2];
    score = std::stoi(strings[3]);
    radius = std::stof(strings[4]);

    //must be last
    switch_index = std::stoi(strings[strings.size() - 1]);
}

void CreatureStats::on_picture_taken(std::shared_ptr<Picture> picture) {
    times_photographed++;

    //save picture if better than current highest picture
    if(!best_picture || picture->get_total_score() > best_picture->get_total_score()) {
        best_picture = picture;
    }
}

Creature::Creature(std::string code_, int number_) : code(code_), number(number_) {
    //populate metadata based on creature_stats
    CreatureStats &creature_stats = creature_stats_map.at(code);
    assert(code == creature_stats.code);
    name = creature_stats.name;
    description = creature_stats.description;
    score = creature_stats.score;
    radius = creature_stats.radius;

    //must be last
    switch_index = creature_stats.switch_index;
}

//add to constructor?
//could make this faster by doing all creatures at once
//deprecated
void Creature::init_transforms (Scene &scene) {
    for (auto &draw : scene.drawables) {
        Scene::Transform &trans = *draw.transform;
        std::string full_code = get_code_and_number();
        if (trans.name.substr(0, 6) == full_code) {
            if (trans.name.length() == 6) {
                transform = &trans;
                drawable = &draw;
            }
            if (trans.name.length() >= 10 && trans.name.substr(7, 3) == "foc") {
                if (trans.name.length() == 8) {
                    focal_point = &trans;
                    std::cout << "found primary focal point:" << trans.name << std::endl;
                } else {
                    std::cout << "found extra focal point:" << trans.name << std::endl;
                }
                focal_points.push_back(&draw);
                draw.render_to_screen = false;
                draw.render_to_picture = false;
            }         
        }
    }
    if(focal_point == nullptr) {
        focal_point = focal_points.front()->transform;
    }
    assert(focal_point != nullptr);
    assert(transform != nullptr);
    assert(focal_points.size() > 0);
}

void Creature::update(float elapsed, float time_of_day) { //movements not synced to animations
    if(animation_player != nullptr) {
        animation_player->update(elapsed);
    }


    //in hindsight I probably should have just done a big if statement but this is more efficient i believe
    switch (switch_index) {
        case 0: { //FLOATER
            if (!animation_player) { break; }
            //floater idle anim
            else if (animation_player->anim.name == "Idle") {
                //gentle float up and down
                const float cycle_time = 3.0f;
                const float height = 0.15f;
                const float distance = sin((time_of_day + 4 * number) / cycle_time) * elapsed * height;
                transform->position += glm::vec3(transform->make_world_to_local() * glm::vec4(0.0f, 0.0f, distance, 0.0f));

                //SFX
                if (!sfx_loop_played && animation_player->position > 0.5f) {
                    if (rand() % 4 == 0) {
                        float random = ((float) rand() / (RAND_MAX));
                        Sound::play_3D(Sound::sample_map->at("FLO_Idle"), 1.0f, glm::vec3(transform->make_local_to_world() * glm::vec4(transform->position, 1.0f)), random / 4.0f + 0.875f, 5.0f);
                    }
                    sfx_loop_played = true;
                } else if (sfx_loop_played && animation_player->position < 0.5f) {
                    sfx_loop_played = false;
                }
            } else if (animation_player->anim.name == "Action1") {
                const glm::vec3 downwards_speed = glm::vec3(0.0f, 0.0f, -0.3f);
                const glm::vec3 angle = glm::normalize(glm::vec3(0.5f, 0.0f, 1.0f));
                const float x = fmod(animation_player->position + 0.8f, 1.0f);
                const float distance = 0.05f;
                float speed = 1.0f - (float)cos((2.0f * M_PI) * pow(x - 1.0f, 2));
                transform->position += distance * speed * angle + downwards_speed * elapsed;

                //SFX
                if(!sfx_loop_played && animation_player->position > 0.4f) {
                    if(sfx_count < 20) {
                        Sound::play_3D(Sound::sample_map->at("FLO_Bounce"), 1.0f, glm::vec3(
                                               transform->make_local_to_world() * glm::vec4(transform->position, 1.0f)),
                                       1.0f + 0.2f * sfx_count, 8.0f);
                        sfx_count++;
                    }
                    sfx_loop_played = true;
                } else if(sfx_loop_played && animation_player->position < 0.4f) {
                    sfx_loop_played = false;
                }
            }
            break;
        }
        default: {

//            std::cout << name << "fell through animation update" << std::endl;
            break;
        }
    }
}

//animations to be triggered when picture is taken of the creature
void Creature::on_picture() {
    switch(switch_index) {
        case 0: { //FLOATER
            play_animation("Action1");
        }
    }
}


void Creature::play_animation(std::string const &anim_name, bool loop, float speed)
{
    //reset sfx variables
    sfx_count = 0;
    sfx_loop_played = false;
    // If current animation is equal to the one currently playing, do nothing
    if (animation_player && anim_name == animation_player->anim.name) return;

    // Try to retrieve creature animation data based on code
    auto animation_set_iter = BoneAnimation::animation_map.find(code);

    if (animation_set_iter == BoneAnimation::animation_map.end())
    {
        throw std::runtime_error("Error: Animation SET not found for creature: " + code);
    }

    // Try to retrive animation data based on animation name
    BoneAnimation * bone_anim_set = animation_set_iter->second;
    BoneAnimation::Animation const * animation = &(bone_anim_set->lookup(anim_name));

    // Check looping or not
    BoneAnimationPlayer::LoopOrOnce loop_or_once = loop ? BoneAnimationPlayer::LoopOrOnce::Loop : BoneAnimationPlayer::LoopOrOnce::Once;

    // If animation is found, set the current animation to the new one
    animation_player = std::make_unique<BoneAnimationPlayer>(*bone_anim_set, *animation, loop_or_once, speed);

    // For that creature, set the current animation to the new one
    drawable->pipeline[Scene::Drawable::ProgramTypeDefault].set_uniforms = [&] () {
        animation_player->set_uniform(bone_lit_color_texture_program->BONES_mat4x3_array);
        glUniform1f(bone_lit_color_texture_program->ROUGHNESS_float, drawable->roughness);
    };
    //set uniforms on shadow pipeline
    drawable->pipeline[Scene::Drawable::ProgramTypeShadow].set_uniforms = [&] () {
        animation_player->set_uniform(bone_shadow_program->BONES_mat4x3_array);
    };
}

glm::vec3 Creature::get_best_angle() const {
    return focal_point->get_front_direction();
}

std::string Creature::get_code_and_number() const {
    return Creature::get_code_and_number(code, number);
}


std::string Creature::get_code_and_number(std::string code, int number) {
    std::string full_code;
    if(number < 10) {
        full_code = code + "_0" + std::to_string(number);
    } else {
        full_code = code + "_" + std::to_string(number);
    }
    return full_code;
}
