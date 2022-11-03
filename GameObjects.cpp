#include "GameObjects.hpp"
#include "Scene.hpp"

std::map< std::string, Creature > Creature::creature_map = std::map< std::string, Creature >();

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

            if (trans.name.substr(trans.name.find('_') + 1, 3) == "foc") {
                std::cout << trans.name << std::endl;
                focal_points.push_back(&draw);
                draw.render_to_screen = false;
                draw.render_to_picture = false;
            }         
        }
    }
    assert(transform != nullptr);
    assert(focal_points.size() > 0);
}

