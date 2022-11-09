#include "GameObjects.hpp"
#include "Scene.hpp"
#include "glm/gtx/string_cast.hpp"

std::map< std::string, Creature > Creature::creature_map = std::map< std::string, Creature >();

//add to constructor?
void Creature::init_transforms (Scene &scene) {
    for (auto &draw : scene.drawables) {
        Scene::Transform &trans = *draw.transform;
        std::cout << trans.name << std::endl;
        std::string full_code;
        if(number < 10) {
            full_code = code + "_0" + std::to_string(number);
        } else {
            full_code = code + "_" + std::to_string(number);
        }
        if (trans.name.substr(0, 6) == full_code) {
            std::cout << trans.name.substr(6) <<std::endl;
            if (trans.name.substr(6) == name) {
                transform = &trans;
                drawable = &draw;
                // std::cout << "transform set to be" << trans.name << std::endl;
            }

            if (trans.name.substr(6, 3) == "foc") {
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

