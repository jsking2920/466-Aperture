#pragma once

#include "GL.hpp"
#include <glm/glm.hpp>

// Framebuffer code, adjusted from: https://github.com/15-466/15-466-f20-framebuffer
// A global set of framebuffers for use in various offscreen rendering effects:
struct Framebuffers {

    // Called to trigger (re-)allocation on window size change
    void realloc(const glm::uvec2 &drawable_size, const glm::uvec2 &new_shadow_size);

    // Current size of framebuffer attachments
    glm::uvec2 size = glm::uvec2(0,0);

    //occlusion checking
    GLuint oc_fb = 0;
    GLuint vertex_position_tex = 0; //stores positions for each fragment

    // MSAA enabled gl objects
    int msaa_samples = 4; // number of samples per pixel for multisample anti-aliasing
    GLuint ms_color_tex = 0; // GL_RGB16F color texture
    GLuint ms_depth_tex = 0; // GL_DEPTH_COMPONENT24 renderbuffer
    GLuint ms_fb = 0; // color0: ms_color_tex , depth: ms_depth_rb

    // Intermediate post-processing objects
    GLuint pp_fb = 0; // intermediate between anti-aliased fb and final render
    GLuint pp_depth = 0;
    GLuint screen_texture = 0;

    // Objects for the "bloom" effect
    GLuint blur_tex = 0; //GL_RGB16F color texture for first pass of blur
    GLuint blur_fb = 0; // color0: blur_x_tex

    //This framebuffer is used for shadow maps, from https://github.com/ixchow/15-466-f18-base3
    glm::uvec2 shadow_size = glm::uvec2(0,0);
    //GLuint shadow_color_tex = 0; //DEBUG
    GLuint shadow_depth_tex = 0;
    GLuint shadow_fb = 0;

    //Objects for depth effects
    GLuint depth_effect_fb = 0;
    GLuint depth_effect_tex = 0;

    //Object for picture transfer = 0;
    GLuint picture_fb = 0;
    GLuint picture_tex = 0;

    void tone_map_to_screen(GLuint texture); //copy ms_color_tex to screen with tone mapping applied
    void tone_map_to_buffer(GLuint texture, GLuint buffer);
    void add_depth_of_field(float focal_distance, glm::vec3 player_pos); //do a basic bloom effect on the screen_texture
    void add_depth_effects(float fog_intensity, float fog_exp, glm::vec3 fog_color);

};

// the actual storage
extern Framebuffers framebuffers;
// NOTE: I could have used a namespace and declared every element extern but that seemed more cumbersome to write
