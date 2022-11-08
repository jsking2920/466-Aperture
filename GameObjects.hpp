#pragma once

#include "Mode.hpp"
#include "Scene.hpp"

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

    //constructor
    Creature() = default;
    Creature(std::string name_, std::string code_, int ID_, int number_)
            : name(name_), code(code_), ID(ID_), number(number_) {}
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
    //the creature ID
    int ID = 0;
    //which number of creature it is
    int number = 0;
    //have a transform which we can query for position and orientation
    Scene::Transform *transform = nullptr;
    //have a drawable for rendering
    Scene::Drawable *drawable = nullptr;
    //have a list of objects to sample
    //If we want to assign points/names to each focal point, make this into a list of focal point objects
    std::vector<Scene::Drawable *> focal_points = {};
    //have an array of body parts
    //std::vector< Scene::Transform > body_parts = {};


    //Initialization/parsing function form scene
    void init_transforms(Scene &scene);
    //Helper functions:
    //return the best angle in world space
    glm::vec3 get_best_angle() const;

    private:
    Scene::Transform *focal_point = nullptr;


};

