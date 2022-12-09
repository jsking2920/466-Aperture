#include "GameObjects.hpp"
#include "Scene.hpp"
#include "glm/gtx/string_cast.hpp"
#include "BoneLitColorTextureProgram.hpp"
#include "ShadowProgram.hpp"
#include "Sound.hpp"

//#define _USE_MATH_DEFINES << gives redefine warning ??
//#define M_2_PI     0.636619772367581343076
#include <math.h>
#include <glm/gtx/quaternion.hpp>

std::map< std::string, Creature > Creature::creature_map = std::map< std::string, Creature >();
std::map< std::string, CreatureStats > Creature::creature_stats_map = std::map< std::string, CreatureStats >();

//code from https://gamedev.stackexchange.com/questions/151823/get-enemy-chaser-object-to-face-player-object-opengl
//THANK YOU SO MUCH THIS CAUSED ME HEADACHES
glm::quat AimAtPoint(glm::vec3 chaserpos, glm::vec3 tgt)
{
    glm::vec3 x = ( tgt - chaserpos );
    x = glm::normalize(x);
// y is z cross x.
    glm::vec3 y = glm::cross(glm::vec3(0,0,1), x);
    y = glm::normalize(y);
// z is x cross y.
    glm::vec3 z = glm::cross(x, y);

    glm::mat4 chasermat = glm::mat4(1.0f);
    chasermat[0] = glm::vec4(x, 0);
    chasermat[1] = glm::vec4(y, 0);
    chasermat[2] = glm::vec4(z, 0);
    chasermat[3] = glm::vec4(chaserpos, 1);

    return glm::toQuat(chasermat);
}

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

glm::vec3 Creature::tan_calculate_pos_at(float time_of_day) {
    glm::vec3 ret;

    //positions
    const glm::vec3 start_pos(-100.f, 100.f, -20.0f);
    const glm::vec3 finished_rising(-86.f, -2.f, 40.f);
    const glm::vec3 circle_one_center(0, -96, 40);
    const float circle_one_radius = 40.f;
    const float circle_one_phase = 0.0f;
    const glm::vec3 finished_charge = glm::vec3(circle_one_center.x + circle_one_radius * sin(1.0 * M_PI * (circle_one_phase)),
                                                circle_one_center.y + circle_one_radius * cos(1.0 * M_PI * (circle_one_phase)), circle_one_center.z);
    const glm::vec3 finished_circle_one = glm::vec3(circle_one_center.x + circle_one_radius * sin(1.0 * M_PI * (1 + circle_one_phase)),
                                                    circle_one_center.y + circle_one_radius * cos(1.0 * M_PI * (1 + circle_one_phase)), circle_one_center.z);
    const glm::vec3 circle_two_center(-67, -32, 40);
    const float circle_two_radius = 60.f;
    const float circle_two_phase = 0.5f;
    const glm::vec3 start_circle_two = glm::vec3(circle_two_center.x + circle_two_radius * sin(1.65f * M_PI * (circle_two_phase)),
                                                 circle_two_center.y + circle_two_radius * cos(1.65f * M_PI * (circle_two_phase)), circle_two_center.z);
    const glm::vec3 finished_circle_two = glm::vec3(circle_two_center.x + circle_two_radius * sin(1.65f * M_PI * (1 + circle_two_phase)),
                                                    circle_two_center.y + circle_two_radius * cos(1.65f * M_PI * (1 + circle_two_phase)), circle_two_center.z);
    const glm::vec3 dive_finished(-53, -32, -20);


    //times
    const float start_time = 196;
    const float finished_rising_time = 202;
    const float finished_charge_time = 206;
    const float finished_circle_one_time = 215;
    const float start_circle_two_time = 218;
    const float finished_circle_two_time = 232;
    const float dive_finished_time = 238;
    if(time_of_day < start_time) {
        ret = start_pos;
    } else if (time_of_day < finished_rising_time) {
        if(animation_player->anim.name == "Idle") {
            play_animation("Action1", false);
            Sound::play_3D(Sound::sample_map->at("TAN_Roar"), 8.0f, transform->make_local_to_world()[3], 1.0f, 1000.f);
        }
        ret = glm::mix(start_pos, finished_rising, (time_of_day - start_time) / (finished_rising_time - start_time));
    } else if (time_of_day < finished_charge_time) {
        ret = glm::mix(finished_rising, finished_charge, (time_of_day - finished_rising_time) / (finished_charge_time - finished_rising_time));
    } else if (time_of_day < finished_circle_one_time) {
        float t = (time_of_day - finished_charge_time) / (finished_circle_one_time - finished_charge_time);
        ret = glm::vec3(circle_one_center.x + circle_one_radius * sin(1.0 * M_PI * (t + circle_one_phase)),
                                  circle_one_center.y + circle_one_radius * cos(1.0 * M_PI * (t + circle_one_phase)), circle_one_center.z);
    } else if (time_of_day < start_circle_two_time) {
        ret = glm::mix(finished_circle_one, start_circle_two, (time_of_day - finished_circle_one_time) / (start_circle_two_time - finished_circle_one_time));
    } else if (time_of_day < finished_circle_two_time) {
        float t = (time_of_day - start_circle_two_time) / (finished_circle_two_time - start_circle_two_time);
        ret = glm::vec3(circle_two_center.x + circle_two_radius * sin(1.65f * M_PI * (t + circle_two_phase)),
                        circle_two_center.y + circle_two_radius * cos(1.65f * M_PI * (t + circle_two_phase)), circle_two_center.z);
    } else if (time_of_day < dive_finished_time) {
        ret = glm::mix(finished_circle_two, dive_finished, (time_of_day - finished_circle_two_time) / (dive_finished_time - finished_circle_two_time));
    } else {
        ret = dive_finished;
    }
    return ret;
}


void Creature::update(float elapsed, float time_of_day, glm::vec3 &player_pos) { //movements not synced to animations
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
                        Sound::play_3D(Sound::sample_map->at("FLO_Idle"), 1.0f, glm::vec3(transform->make_local_to_world() * glm::vec4(transform->position, 1.0f)), random / 4.0f + 0.875f, 10.0f);
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
                        Sound::play_3D(Sound::sample_map->at("FLO_Bounce"), 1.0f,
                                               transform->make_local_to_world()[3],
                                       1.0f + 0.2f * sfx_count, 12.0f);
                        sfx_count++;
                    }
                    sfx_loop_played = true;
                } else if(sfx_loop_played && animation_player->position < 0.4f) {
                    sfx_loop_played = false;
                }
            }
            break;
        }
        case 1: { //MEEPER
            if (!animation_player) { break; }
            //move towards home, unless
            //random chance to move towards home
            if(animation_player->anim.name == "Idle") {
                if(!bool_flag && fmod(time_of_day * (float)number, 1.0f) < 0.5f) {
                    if(rand() % 3 == 0) {
                        //move towards home
                        if(transform->position != original_pos) {
                            glm::vec3 randomness = 0.4f * glm::normalize(glm::vec3((float)rand() / (float)RAND_MAX - 0.5f, (float)rand() / (float)RAND_MAX - 0.5f, ((float)rand() / (float)RAND_MAX) - 0.5f) * 0.1f);
                            glm::vec3 diff = glm::normalize(original_pos - transform->position) + randomness;
                            glm::vec3 clampedDiff = glm::vec3(diff.x, diff.y, glm::clamp(diff.z, -0.2f, 0.2f));
                            glm::vec3 endPoint = transform->position + clampedDiff;
                            glm::quat direction = AimAtPoint(transform->position, endPoint);
                            transform->rotation = direction;
                        }
                        //randomness
                        play_animation("Action1", false);
                    }
                    //sfx
                    if(rand() % 4 == 0) {
                        float random = ((float) rand() / (RAND_MAX));
                        Sound::play_3D(Sound::sample_map->at("MEP_Idle"), 2.0f, transform->make_local_to_world()[3], random / 1.5f + 0.7f, 15.f);
                    }
                    bool_flag = true;
                } else if (bool_flag && fmod(time_of_day, 1.0f) > 0.5f) {
                    bool_flag = false;
                }
            } else { //moving
                //check for animation finished
                if(animation_player->done()) {
                    play_animation("Idle", true);
                } else {
                    //move
                    float speed = (0.5f + ((float)rand() / (float)RAND_MAX)) * (float)(1 - cos(2 * M_PI * animation_player->position));
                    glm::vec3 direction = glm::rotate(transform->rotation, (glm::vec3(1.0f, 0.0f, 0.0f)));
                    transform->position +=
                            speed * direction *
                            elapsed;
                }
            }
            break;
        }
        case 2: { //TAN
            if (!animation_player) { break; }
            const float smooth = 2.f;
            glm::vec3 new_pos = tan_calculate_pos_at(time_of_day - elapsed + smooth);
            transform->rotation = AimAtPoint(transform->position, new_pos);
            transform->position += (new_pos - transform->position) * elapsed;
            //random chance to roar
            if(animation_player->anim.name == "Idle" && time_of_day > 195.5f) {
                if(!bool_flag && fmod(time_of_day, 1.0f) < 0.5f) {
                    if(rand() % 8 == 0) {
                        play_animation("Action1", false);
                        Sound::play_3D(Sound::sample_map->at("TAN_Roar"), 5.0f, transform->make_local_to_world()[3], 1.0f, 1000.f);
                    }
                    bool_flag = true;
                } else if (bool_flag && fmod(time_of_day, 1.0f) > 0.5f) {
                    bool_flag = false;
                }
            } else { //roaring
                //check for animation finished
                if(animation_player->done()) {
                    play_animation("Idle", true);
                }
            }
            break;
        }
        case 3: { //TRI
            if (!animation_player) { break; }
            break;
        }
        case 4: { //SNA
            if (!animation_player) { break; }
            //bool_flag is 0 if out of distance, 1 if in distance
            const float scared_distance = 22.0f;
            bool_flag = glm::length(player_pos - transform->make_local_to_world()[3]) < scared_distance;
            if (bool_flag && animation_player->anim.name == "Idle") {
                play_animation("Action1", false);
                float random = ((float) rand() / (RAND_MAX));
                Sound::play_3D(Sound::sample_map->at("SNA_Hide"), 2.5f, glm::vec3(transform->make_local_to_world()[3]), random / 4.0f + 0.875f, 40.0f);

            } else if(bool_flag && animation_player->anim.name == "Action1") {
                if(animation_player->position > 0.5f) {
                    animation_player->set_speed(0.0f);
                }
            } else if (!bool_flag && animation_player->anim.name == "Action1") {
                if(animation_player->position <= 0.02f) {
                    //coming out animation over
                    play_animation("Idle");
                } else {
                    animation_player->set_speed(-1.0f);
                }
            }
            break;
        }
        case 5: { //PEN
            if (!animation_player) { break; }
            //bool_flag is 0 if out of distance, 1 if in distance
            const float angry_distance = 5.0f;
            bool_flag = glm::length(player_pos - transform->make_local_to_world()[3]) < angry_distance;
            if(bool_flag) {
                //Todo: make smooth
                transform->rotation = AimAtPoint(glm::vec3(glm::vec2(transform->make_local_to_world()[3]), 0), glm::vec3(player_pos.x, player_pos.y, 0.f));
                if(animation_player->anim.name == "Idle" || animation_player->done()) { //manual loop to ensure smooth transitions
                    play_animation("Action1", false);
                    float random = ((float) rand() / (RAND_MAX));
                    Sound::play_3D(Sound::sample_map->at("PEN_Angry"), 1.0f, glm::vec3(transform->make_local_to_world()[3]), random / 4.0f + 0.875f, 15.0f);
                }
            } else if(!bool_flag) {
                if(animation_player->done()) {
                    play_animation("Idle");
                }

                if(!sfx_loop_played && animation_player->position > 0.4f) {
                    float random_chooser = ((float) rand() / (RAND_MAX));
                    if(random_chooser > 0.5f) {
                        float random = ((float) rand() / (RAND_MAX));
                        Sound::play_3D(Sound::sample_map->at("PEN_Idle"), 1.0f, glm::vec3(transform->make_local_to_world()[3]), random / 4.0f + 0.875f, 18.0f);
                    } else {
                        float random = ((float) rand() / (RAND_MAX));
                        Sound::play_3D(Sound::sample_map->at("PEN_Idle_2"), 1.0f, glm::vec3(transform->make_local_to_world()[3]), random / 4.0f + 0.875f, 18.0f);
                    }
                    sfx_loop_played = true;
                } else if(sfx_loop_played && animation_player->position < 0.4f) {
                    sfx_loop_played = false;
                }

            }
            break;
        }
        default: {
            std::cout << name << " fell through animation update" << std::endl;
            break;
        }
    }
}

//animations to be triggered when picture is taken of the creature
void Creature::on_picture(glm::vec3 player_to_creature) {
    switch(switch_index) {
        case 0: { //FLOATER
            play_animation("Action1");
            break;
        }
        case 3: { //TRI
            float random = ((float) rand() / (RAND_MAX));
            Sound::play_3D(Sound::sample_map->at("TRI_Idle"), 1.0f, glm::vec3(transform->make_local_to_world()[3]), random / 4.0f + 0.875f, 8.0f);
//            glm::vec3 player_pos = transform->make_local_to_world()[3] + (-player_to_creature);
            transform->rotation = AimAtPoint(glm::vec3(glm::vec2(transform->make_local_to_world()[3]), 0), glm::vec3(player_pos.x, player_pos.y, 0.f));
            if(animation_player->anim.name == "Idle" || animation_player->done()) { //manual loop to ensure smooth transitions
                play_animation("Action1", false);
            }
        }
        case 5: { //PEN
//            glm::vec3 player_pos = transform->make_local_to_world()[3] + (-player_to_creature);
            transform->rotation = AimAtPoint(glm::vec3(glm::vec2(transform->make_local_to_world()[3]), 0), glm::vec3(player_pos.x, player_pos.y, 0.f));
            if(animation_player->anim.name == "Idle" || animation_player->done()) { //manual loop to ensure smooth transitions
                play_animation("Action1", false);
            }
        }

    }
}


void Creature::play_animation(std::string const &anim_name, bool loop, float speed)
{
    //reset sfx variables
    sfx_count = 0;
    sfx_loop_played = false;
    // If current animation is equal to the one currently playing, set speed only
    if (animation_player && anim_name == animation_player->anim.name && !animation_player->done()) {
        animation_player->set_speed(speed);
        return;
    }

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

void Creature::reset() {
    original_pos = glm::vec3(0.f);;
    bool_flag = false;
    sfx_loop_played = false;
    sfx_count = 0;

    transform->position = original_pos;
    play_animation("Idle", true, 1.0f);
}
