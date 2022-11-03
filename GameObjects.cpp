#include "GameObjects.hpp"
#include "Scene.hpp"

//constructor
Creature::Creature() {
    creature_map.insert(std::pair< std::string, Creature& >(name, *this));
}

Creature::Creature(std::string name_, std::string code_, int ID_, int number_, glm::vec3 ideal_angle_)
: name(name_), code(code_), ID(ID_), number(number_), ideal_angle(ideal_angle_) {
    //add to creature map
    creature_map.insert(std::pair< std::string, Creature& >(name, *this));
}

void Creature::init_transforms (Scene &scene) {

    for (auto &draw : scene.drawables) {
        Scene::Transform &trans = *draw.transform;
        if (trans.name.substr(0, 4) == (code + std::to_string(number))) {
            if (trans.name.substr(5) == name) {
                transform = &trans;
                drawable = &draw;
                // std::cout << "transform set to be" << trans.name << std::endl;
            }

            if (trans.name.substr(name.length() + 1, 3) == "foc") {
                // std::cout << trans.name << std::endl;
                focal_points.push_back(&draw);
            }         
        }
    }
    assert(transform != nullptr);
    assert(focal_points.size() > 0);
}

