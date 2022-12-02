#pragma once

#include "Mode.hpp"
#include "Scene.hpp"
#include "BoneAnimation.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <map>

/*
33 characters and then a number
*/

//idea: make a mast struct/class that's the creature
struct Creature {
    // Reference to the creatures
    static std::map< std::string, Creature > creature_map;
    static std::map< std::string, std::vector < std::string > > creature_stats_map;

    //constructor
    Creature() = default;
    Creature(std::string code_, int number_);
    ~Creature() = default;

    //Displaying information to the user --------------------------------------
    //TODO: best to simply this into a struct later, possibly populated from a struct
    /*
    std::string name = "Creature";
    std::string description = "looks great!";
    */

    //METADATA ----------------------------------------------------
    //name of the creature
    std::string name = "unnamed";
    //three character code for parsing the creature
    std::string code = "missing code";
    //which number of creature it is
    int number = 0;
    std::string description = "missing description";
    float radius = 0.f; //radius, to be subtracted when calculating focus
    //have a transform which we can query for position and orientation
    Scene::Transform *transform = nullptr;
    //have a drawable for rendering
    Scene::Drawable *drawable = nullptr;
    //have a list of objects to sample
    //If we want to assign points/names to each focal point, make this into a list of focal point objects
    std::vector<Scene::Drawable *> focal_points = {};
    //Holds current animation - unique ptr to automatically dispose of old animations
    std::unique_ptr< BoneAnimationPlayer > animation_player;
    //keep track of animation state
    std::string curr_anim_name = "none";
    //index for switch statement, bc you can't switch on strings
    int switch_index = 0;

    //scoring parameters
    int score = 3000;

    void update(float elapsed, float time_of_day);
    void on_picture();

    //Initialization/parsing function form scene
    void init_transforms(Scene &scene);
    //Helper functions:
    //return the best angle in world space
    glm::vec3 get_best_angle() const;
    std::string get_code_and_number() const;
    static std::string get_code_and_number(std::string code, int number);

    Scene::Transform *focal_point = nullptr;


};

