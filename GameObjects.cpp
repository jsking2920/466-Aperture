#include "GameObjects.hpp"
#include "Scene.hpp"

//add to constructor?
void Creature::init_transforms (Scene &scene) {

    for (auto &draw : scene.drawables) {
        Scene::Transform &trans = *draw.transform;
        if (trans.name.substr(0, 4) == (code + std::to_string(number))) {
            if (trans.name.substr(5) == name) {
                transform = &trans;
                drawable = &draw;
                // std::cout << "transform set to be" << trans.name << std::endl;
            }
            std::cout << trans.name << std::endl;

            if (trans.name.substr(code.length() + std::to_string(ID).length() + 1, 3) == "foc") {
                focal_points.push_back(&draw);
            }         
        }
    }
    assert(transform != nullptr);
    assert(focal_points.size() > 0);
}

