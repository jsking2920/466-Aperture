#include "GameObjects.hpp"
#include "Scene.hpp"

//constructor 
void Creature::init_transforms (Scene &scene) {

    for (auto &trans : scene.transforms) {
        if (trans.name.substr(0, 4) == (code + std::to_string(number))) {
            if (trans.name.substr(5) == name) {
                transform = &trans;
                // std::cout << "transform set to be" << trans.name << std::endl;
            }

            if (trans.name.substr(5, 3) == "foc") {
                // std::cout << trans.name << std::endl;
                focal_points.push_back(&trans);
            }         
        }
    }
    assert(transform != nullptr);
    assert(focal_points.size() > 0);
}