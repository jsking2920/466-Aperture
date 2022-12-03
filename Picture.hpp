#pragma once

#include "Scene.hpp"
#include "GameObjects.hpp"
#include <glm/glm.hpp>

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
    
    PictureCreatureInfo creatureInfo;

    glm::vec2 dimensions;
    std::shared_ptr<std::vector<GLfloat>> data;

    float focal_distance;
    glm::vec3 angle;
    uint32_t total_frag_count;
    std::list< std::pair<Scene::Drawable&, GLuint > > frag_counts;
    std::list< PictureCreatureInfo > creatures_in_frame;
};

struct Picture {

    Picture() = default;
    explicit Picture(PictureInfo &stats);

    glm::vec2 dimensions;
    // TODO: we probably don't need both of these
    std::shared_ptr<std::vector<GLfloat>> data; // used to save pic to png
    GLuint tex = 0; // used to draw picture

    std::string title; //generate a silly title based on subject of title + adjective?
    std::list<ScoreElement> score_elements;

    std::list<ScoreElement> score_creature(PictureCreatureInfo &creature_info, PictureInfo &picture_info);
    uint32_t get_total_score();
    std::string get_scoring_string();
    void save_picture_png(); // saves picture as a png to dist/PhotoAlbum/ (creating folder if needed)

    static const std::string adjectives[];
};

// TODO: currently one of these is getting constructed for every picture being drawn in a frame
// Should probably have one struct handle a list of pictures to draw, and maybe cache those resources to avoid setting everything up for every pic every frame
// Based on DrawSprites
struct DrawPicture {

    Picture const &pic;
    glm::uvec2 drawable_size;
    glm::mat4 to_clip;

    struct Vertex {
        Vertex(glm::vec2 const& Position_, glm::vec2 const& TexCoord_, glm::u8vec4 const& Color_) : Position(Position_), TexCoord(TexCoord_), Color(Color_) { }
        glm::vec2 Position;
        glm::vec2 TexCoord;
        glm::u8vec4 Color;
    };
    std::vector< Vertex > attribs;

    // Prepare aresources for a pic to be drawn
    DrawPicture(Picture const& pic, glm::uvec2 const& drawable_size);
    // Tell DrawPicture to draw the picture with given parameters
    void draw(glm::vec2 const& center, float scale = 1.0f, glm::u8vec4 const& tint = glm::u8vec4(0xff, 0xff, 0xff, 0xff));
    // Actually draws the picture on deallocation
    ~DrawPicture();
};
