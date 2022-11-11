#include "GameObjects.hpp"
#include "Scene.hpp"
#include "glm/gtx/string_cast.hpp"

std::map< std::string, Creature > Creature::creature_map = std::map< std::string, Creature >();
std::map< std::string, std::vector < std::string > > Creature::creature_stats_map = std::map< std::string, std::vector < std::string > >();


Creature::Creature(std::string code_, int number_) : code(code_), number(number_) {
    //populate metadata based on creature_stats
    std::vector < std::string > &creature_stats = creature_stats_map[code];
    assert(code == creature_stats[0]);
    assert(creature_stats.size() == 4); //if assert fails, you probably added a field to the csv that must be added here!
    name = creature_stats[1];
    description = creature_stats[2];
    score = std::stoi(creature_stats[3]);
}

//add to constructor?
//could make this faster by doing all creatures at once
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

