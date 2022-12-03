#include "GameObjects.hpp"
#include "Scene.hpp"
#include "glm/gtx/string_cast.hpp"
#include "BoneLitColorTextureProgram.hpp"
#include "ShadowProgram.hpp"

std::map< std::string, Creature > Creature::creature_map = std::map< std::string, Creature >();
std::map< std::string, std::vector < std::string > > Creature::creature_stats_map = std::map< std::string, std::vector < std::string > >();


Creature::Creature(std::string code_, int number_) : code(code_), number(number_) {
    //populate metadata based on creature_stats
    std::vector < std::string > &creature_stats = creature_stats_map[code];
    assert(code == creature_stats[0]);
    assert(creature_stats.size() == 6); //if assert fails, you probably added a field to the csv that must be added here!
    name = creature_stats[1];
    description = creature_stats[2];
    score = std::stoi(creature_stats[3]);
    radius = std::stof(creature_stats[4]);

    //must be last
    switch_index = std::stoi(creature_stats[creature_stats.size() - 1]);
}

//add to constructor?
//could make this faster by doing all creatures at once
//deprecated
void Creature::init_transforms (Scene &scene) {
    for (auto &draw : scene.drawables) {
        Scene::Transform &trans = *draw.transform;
        std::string full_code = get_code_and_number();
//        std::cout << trans.name.substr(0, 6) << ", " << full_code << std::endl;
        if (trans.name.substr(0, 6) == full_code) {
//            std::cout << trans.name.substr(6) <<std::endl;
            if (trans.name.length() == 6) {
                transform = &trans;
                drawable = &draw;
                // std::cout << "transform set to be" << trans.name << std::endl;
            }

//            std::cout << trans.name.substr(7, 3) <<std::endl;
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
    animation_player->update(elapsed);

    //in hindsight I probably should have just done a big if statement but this is more efficient i believe
    switch(switch_index) {
        case 0: { //FLOATER
            if(!animation_player) { break; }
            //floater idle anim
            else if(animation_player->anim.name == "Idle") {
                //gentle float up and down
                const float cycle_time = 3.f;
                const float height = 0.15f;
                const float distance = sin((time_of_day + 4 * number) / cycle_time) * elapsed * height;
                transform->position += glm::vec3(transform->make_world_to_local() * glm::vec4(0.0f, 0.f, distance, 0.0f));
            } else if(animation_player->anim.name == "Action1") {
                const glm::vec3 downwards_speed = glm::vec3(0.f, 0.f, -0.3f);
                const glm::vec3 angle = glm::normalize(glm::vec3(0.5f, 0.f, 1.0f));
                const float x = fmod(animation_player->position + 0.6f, 1);
                const float distance = 0.6f;
                const float speed = 1 - cos(M_2_PI * pow(x - 1, 2));
                transform->position += distance * speed * angle + downwards_speed * elapsed;
            }
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
    // If current animation is equal to the one currently playing, do nothing
    if (animation_player && anim_name == animation_player->anim.name) return;

    // Try to retrieve creature animation data based on code
    auto animation_set_iter = BoneAnimation::animation_map.find(code);

    if (animation_set_iter == BoneAnimation::animation_map.end())
    {
        throw std::runtime_error("Error: Animation SET not found for creature: " + code);
    }

    // Try to retrive animation data based on animation name
    BoneAnimation *bone_anim_set = animation_set_iter->second;
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

