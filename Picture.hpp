#pragma once

#include "Scene.hpp"
#include "GameObjects.hpp"
#include <glm/glm.hpp>
#include <iostream>

struct Creature;

struct ScoreElement {

    ScoreElement(std::string _name, uint32_t _value) : name(_name), value(_value) {};
    std::string name = "Amazing";
    uint32_t value = 1;
};

struct PictureCreatureInfo {

    Creature* creature;
    float frag_count;
    glm::vec3 player_to_creature;
    std::vector<bool> are_focal_points_in_frame;
};

//data about the picture that is used for scoring
struct PictureInfo {
    glm::vec2 dimensions;
    std::shared_ptr<std::vector<GLfloat>> data;

    float focal_distance;
    glm::vec3 angle;
    uint32_t total_frag_count;
    std::list< std::pair<Scene::Drawable&, GLuint > > frag_counts;
    std::list< PictureCreatureInfo > creatures_in_frame;
};

struct Picture {
    
    PictureInfo pictureInfo;
    PictureCreatureInfo subject_info;


    Picture() = default;
    Picture(const Picture&) { std::cout << "yes\n"; }
    explicit Picture(PictureInfo &stats);

    glm::vec2 dimensions;
    std::shared_ptr<std::vector<GLfloat>> data;

    std::string title; //generate a silly title based on subject of title + adjective?
    std::list<ScoreElement> score_elements;

    std::list<ScoreElement> score_creature(PictureCreatureInfo &creature_info, PictureInfo &picture_info);
    uint32_t get_total_score();
    std::string get_scoring_string();
    void save_picture_png(); // saves picture as a png to dist/album/ (creating folder if needed)

    static const std::string adjectives[];
};
