#pragma once

#include "Scene.hpp"
#include "GameObjects.hpp"
#include <glm/glm.hpp>

struct ScoreElement {
    ScoreElement(std::string _name, uint32_t _value) : name(_name), value(_value) {};
    std::string name = "Amazing";
    uint32_t value = 1;
};

struct Picture {
    struct PictureStatistics { //data about the picture that is used for scoring
        glm::vec2 dimensions;
        std::vector<GLfloat> data;
        std::list<std::pair<Scene::Drawable &, GLuint>> frag_counts;
        std::list< std::pair<Creature *, std::shared_ptr< std::vector< bool > > > > creatures_in_frame;
    };

    Picture() = default;
    explicit Picture(PictureStatistics stats);

    glm::vec2 dimensions;
    std::vector<GLfloat> data;

    std::string title; //generate a silly title based on subject of title + adjective?
    std::list<ScoreElement> score_elements;

    uint32_t get_total_score();
    std::string get_scoring_string();
    void save_picture_png(); // saves picture as a png to dist/album/ (creating folder if needed)
};
