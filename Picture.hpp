#pragma once

#include "Scene.hpp"
#include <glm/glm.hpp>

struct ScoreElement {
    ScoreElement(std::string _name, uint32_t _value) : name(_name), value(_value) {};
    std::string name = "Amazing";
    uint32_t value = 1;
};

//Picture() is intended to mostly function as data - functions with effects should go elsewhere
struct Picture {
    Picture();

    std::string title; //generate a silly title based on subject of title + adjective?
    std::list<ScoreElement> score_elements;

    uint32_t get_total_score();
    std::string get_scoring_string();
};
