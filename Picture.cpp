#include "Picture.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"

#include "ColorTextureProgram.hpp"
#include "Load.hpp"
#include "GL.hpp"
#include <glm/glm.hpp>
#include "gl_errors.hpp"
#include "Framebuffers.hpp"
#include "gl_check_fb.hpp"
// for glm::value_ptr()
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <utility>
#include <filesystem>
#include <math.h>

const std::string Picture::adjectives[] = { "Majestic", "Beautiful", "Vibrant", "Grand", "Impressive", "Awe-inspiring", "Sublime", "Resplendent", "Striking", "Marvelous", "Alluring", "Gorgeous", "Glamorous", "Divine", "Exquisite", "Stunning", "Beauteous", "Breathtaking", "Pulchritudinous", "Graceful", "Dazzling", "Lovely", "Superb", "Resplendent", "Radiant", "Well-formed", "Great"};

Picture::Picture(PictureInfo &stats) : dimensions(stats.dimensions), data(stats.data) {
    if (stats.frag_counts.empty()) {
        title = "Pure Emptiness";
        score_elements.emplace_back("Relatable", (uint32_t)500);
        score_elements.emplace_back("Deep", (uint32_t)500);
    } else if (stats.creatures_in_frame.empty()) {
        //TODO: once we add in some points for nature, make this better
        title = "Beautiful Nature";
        score_elements.emplace_back("So peaceful!", (uint32_t)2000);
    } else {

    subject_info = stats.creatures_in_frame.front();

        //grade subject
        {
            //Magnificence
            score_elements.emplace_back(subject_info.creature->name, subject_info.creature->score);
            auto result = score_creature(subject_info, stats);
            score_elements.insert(score_elements.end(), result.begin(),
                                  result.end()); //from https://stackoverflow.com/q/1449703
        }


    {
        //Add bonus points for additional subjects
        std::for_each(std::next(stats.creatures_in_frame.begin()), stats.creatures_in_frame.end(),
                      [&](PictureCreatureInfo creature_info) {
                          auto result = score_creature(creature_info, stats);
                          int total_score = creature_info.creature->score;
                          std::for_each(result.begin(), result.end(), [&](ScoreElement el) { total_score += el.value; });
                          score_elements.emplace_back("Bonus " + creature_info.creature->transform->name, (uint32_t)(total_score / 10));
                      });
    }

        title = adjectives[rand() % adjectives->size()] + " " + subject_info.creature->name;


        //trigger on_picture behaviors of subject (could be all creatures in frame)
        if(glm::length(subject_info.player_to_creature) < std::max(10.f, 8 * subject_info.creature->radius)) {
            subject_info.creature->on_picture();
        }
    }
    // Create a texture for this picture, to be used for drawing it
    {
        // Upload the texture data to the GPU

        // Generate a new texture object name
        glGenTextures(1, &tex);

        // Bind the new texture object
        glBindTexture(GL_TEXTURE_2D, tex);

        // init the texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)dimensions.x, (GLsizei)dimensions.y, 0, GL_RGB, GL_FLOAT, data->data());

        // Set filtering and wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);

        //attach texture to picture_fb and tone map to it
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers.picture_fb);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        gl_check_fb(); //performance?
        framebuffers.tone_map_to_screen(framebuffers.picture_fb);

        // Unbind the texture object:
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

std::list<ScoreElement> Picture::score_creature(PictureCreatureInfo &creature_info, PictureInfo &stats) {
    std::list<ScoreElement> result;
    {
        //Add points for bigness
        //in the future, could get these params from the creature itself or factor in distance
        const float max_frag_percent = 1.0f / 5.0f;
        const float min_frag_percent = 1.0f / 50.0f;
        const float exponent = 0.7f;
        const float offset = 300;

        float fraction = (float)creature_info.frag_count/(float)stats.total_frag_count;
        //stretch 0 to max_percent to be 0.f to 1.f
        float remapped = std::pow(std::clamp(fraction / max_frag_percent, 0.0f, 1.0f), exponent);
        if (fraction > min_frag_percent && remapped > 0.15f) {
            result.emplace_back("Prominence", (uint32_t)((remapped * 2000.0f + offset) - offset));
        }
    }

    {
        //Add points for focal points
        //in the future, could weight focal points or have per-focal point angles
        int total_fp = (int)creature_info.are_focal_points_in_frame.size();
        int fp_in_frame = (int)std::count_if(creature_info.are_focal_points_in_frame.begin(), creature_info.are_focal_points_in_frame.end(), [](bool b) { return b; });

        float total = (float)fp_in_frame / (float)total_fp * 2000.0f;
        //random salting, could be removed (is this called salting)
        if (total < 2000.0f) {
            total += ((float) rand() / (float)RAND_MAX) * 30.0f;
        }
        result.emplace_back("Anatomy", (uint32_t)total);
    }

    {
        //Add points for angle
        //in the future, change to a multiplier? and also change const params to be per creature
        const float best_degrees_deviated = 7.0f; //degrees away from the ideal angle for full points, keep in mind it's on a cos scale so not linear
        const float worst_degrees_deviated = 90.0f; //degrees away from the ideal angle for min points
        const float exponent = 1.1f;

        glm::vec3 creature_to_player_norm = glm::normalize(-creature_info.player_to_creature);
        //ranges from -1, pointing opposite the correct angle, to 1, pointing directly at the correct angle
        float dot = glm::dot(creature_to_player_norm, creature_info.creature->get_best_angle()); //cos theta
        //clamped between min and max values
        float lo = (float)std::cos( worst_degrees_deviated * M_PI / 180.0f);
        float hi = (float)std::cos(best_degrees_deviated * M_PI / 180.0f);
        float clamped_dot = std::clamp(dot, lo, hi);
        //normalized to be between 0 and 1
        float normalized_dot =  (clamped_dot - lo) / (hi - lo);
        if (normalized_dot > 0.01f) {
            result.emplace_back("Angle", (uint32_t) (std::pow(normalized_dot, exponent) * 3000.0f));
        }
    }

    { //Add points for focus
        float distance = glm::length(creature_info.player_to_creature) - creature_info.creature->radius;
        //calculate blur using same calculation as blur shader in Framebuffers.cpp
        float percent;
        if (distance < stats.focal_distance) {
            percent = 1 - std::clamp(1 - (float)pow(distance/stats.focal_distance, 2), 0.f, 1.f);
        } else {
            percent = 1 - std::clamp((abs(stats.focal_distance - distance) - stats.focal_distance)/(2 * stats.focal_distance), 0.f, 1.f);
        }
        float score = 4000.0f * percent;
        if (score > 1200.f) {
            result.emplace_back("Focus", (uint32_t)score);
        }
        //bonus points for shallow depth of field
        if(score > 2500.f && stats.focal_distance < 5.f) {
            result.emplace_back("Depth of Field!", (uint32_t)((5.f - stats.focal_distance) * 500.f));
        }
    }

    return result;
}

uint32_t Picture::get_total_score() {
    uint32_t total = 0;
    std::for_each(score_elements.begin(), score_elements.end(), [&](ScoreElement el) {
        total += el.value;
    });
    return total;
}

std::string Picture::get_scoring_string() {
    std::string ret = title + "\n";
    std::for_each(score_elements.begin(), score_elements.end(), [&](ScoreElement el) {
       ret += el.name + ": +" + std::to_string(el.value) + "\n";
    });
    ret += "Total Score: " + std::to_string(get_total_score()) + "\n\n";
    return ret;
}


void Picture::save_picture_png() {

    //convert pixel data to correct format for png export
    std::vector<uint8_t>png_data(4 * (unsigned long)dimensions.x * (unsigned long)dimensions.y);
    for (uint32_t i = 0; i < dimensions.x * dimensions.y; i++) {
        png_data[i * 4] = (uint8_t)round((*data)[i * 3] * 255);
        png_data[i * 4 + 1] = (uint8_t)round((*data)[i * 3 + 1] * 255);
        png_data[i * 4 + 2] = (uint8_t)round((*data)[i * 3 + 2] * 255);
        png_data[i * 4 + 3] = 255;
    }
    // create album folder if it doesn't exist
    if (!std::filesystem::exists(data_path("PhotoAlbum/"))) {
        std::filesystem::create_directory(data_path("PhotoAlbum/"));
    }
    //save pic if name is unique
    if (!std::filesystem::exists(data_path("PhotoAlbum/" + title + ".png"))) {
        save_png(data_path("PhotoAlbum/" + title + ".png"), dimensions,
                 reinterpret_cast<const glm::u8vec4*>(png_data.data()), LowerLeftOrigin);
    } else {
        //enumerate file name
        for(uint16_t count = 1; count < 9999; count++) {
            if (!std::filesystem::exists(data_path("PhotoAlbum/" + title + std::to_string(count) + ".png"))) {
                save_png(data_path("PhotoAlbum/" + title + std::to_string(count) + ".png"), dimensions,
                         reinterpret_cast<const glm::u8vec4*>(png_data.data()), LowerLeftOrigin);
                break;
            }
        }
    }
}

// Picture drawing stuff (Based on DrawSprites)

// All DrawPicture instances share a vertex array object and vertex buffer, initialized at load time:
// n.b. declared static so they don't conflict with similarly named global variables elsewhere:
static GLuint vertex_buffer = 0;
static GLuint vertex_buffer_for_color_texture_program = 0;

static Load< void > setup_buffers(LoadTagDefault, []() {

    // Set up vertex buffer:
    {
        glGenBuffers(1, &vertex_buffer); // for now, buffer will be un-filled.
    }

    // Vertex array mapping buffer for color_texture_program
    {
        // ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
        glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

        // set vertex_buffer_for_color_texture_program as the current vertex array object:
        glBindVertexArray(vertex_buffer_for_color_texture_program);

        //set vertex_buffer as the source of glVertexAttribPointer() commands:
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

        //set up the vertex array object to describe arrays of PongMode::Vertex:
        glVertexAttribPointer(
            color_texture_program->Position_vec4, //attribute
            2, //size
            GL_FLOAT, //type
            GL_FALSE, //normalized
            sizeof(DrawPicture::Vertex), //stride
            (GLbyte*)0 + offsetof(DrawPicture::Vertex, Position) //offset
        );
        glEnableVertexAttribArray(color_texture_program->Position_vec4);
        //[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

        glVertexAttribPointer(
            color_texture_program->TexCoord_vec2, //attribute
            2, //size
            GL_FLOAT, //type
            GL_FALSE, //normalized
            sizeof(DrawPicture::Vertex), //stride
            (GLbyte*)0 + offsetof(DrawPicture::Vertex, TexCoord) //offset
        );
        glEnableVertexAttribArray(color_texture_program->TexCoord_vec2);

        glVertexAttribPointer(
            color_texture_program->Color_vec4, //attribute
            4, //size
            GL_UNSIGNED_BYTE, //type
            GL_TRUE, //normalized
            sizeof(DrawPicture::Vertex), //stride
            (GLbyte*)0 + offsetof(DrawPicture::Vertex, Color) //offset
        );
        glEnableVertexAttribArray(color_texture_program->Color_vec4);

        //done referring to vertex_buffer, so unbind it:
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //done setting up vertex array object, so unbind it:
        glBindVertexArray(0);
    }

    GL_ERRORS(); //PARANOIA: make sure nothing strange happened during setup
  });

DrawPicture::DrawPicture(Picture const& pic_, glm::uvec2 const& drawable_size_) : pic(pic_), drawable_size(drawable_size_) {

    glm::vec2 window_min, window_max;

    if (pic.dimensions.x * drawable_size.y < drawable_size.x * pic.dimensions.y) {
        //...need to stretch wider to match aspect:
        float w = pic.dimensions.y * float(drawable_size.x) / float(drawable_size.y);
        window_min.x = 0.5f * pic.dimensions.x - 0.5f * w;
        window_max.x = 0.5f * pic.dimensions.x + 0.5f * w;
        window_min.y = 0.0f;
        window_max.y = pic.dimensions.y;
    }
    else {
        //...need to stretch taller to match aspect:
        window_min.x = 0.0f;
        window_max.x = pic.dimensions.x;
        float h = pic.dimensions.x * float(drawable_size.y) / float(drawable_size.x);
        window_min.y = 0.5f * pic.dimensions.y - 0.5f * h;
        window_max.y = 0.5f * pic.dimensions.y + 0.5f * h;
    }

    glm::vec2 scale = glm::vec2(2.0f) / (window_max - window_min);
    to_clip = glm::mat4( //n.b. column major(!)
        scale.x, 0.0f, 0.0f, 0.0f,
        0.0f, scale.y, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        scale.x * -0.5f * (window_min.x + window_max.x), scale.y * -0.5f * (window_min.y + window_max.y), 0.0f, 1.0f
    );

    //DEBUG: std::cout << glm::to_string(to_clip) << std::endl;
}

void DrawPicture::draw(glm::vec2 const& center, float scale, glm::u8vec4 const& tint) {

    //glm::vec2 min = center + scale * (sprite.min_px - sprite.anchor_px);
    //glm::vec2 max = center + scale * (sprite.max_px - sprite.anchor_px);
    //glm::vec2 min_tc = sprite.min_px / glm::vec2(atlas.tex_size);
    //glm::vec2 max_tc = sprite.max_px / glm::vec2(atlas.tex_size);

    // Simplified from above since we're using a hardcoded anchor and an entire texture instead of a section of one
    glm::vec2 pic_anchor = glm::vec2(pic.dimensions.x / 2.0f, pic.dimensions.y / 2.0f); // Anchor for pictures is the center
    glm::vec2 min = center + scale * (-1.0f * pic_anchor);
    glm::vec2 max = center + scale * (pic.dimensions - pic_anchor);
    glm::vec2 min_tc = glm::vec2(0.0f); // lower left corner of tex
    glm::vec2 max_tc = glm::vec2(1.0f); // uper right corner of tex

    // Split rectangle into two triangles:
    attribs.emplace_back(glm::vec2(min.x, min.y), glm::vec2(min_tc.x, min_tc.y), tint);
    attribs.emplace_back(glm::vec2(max.x, min.y), glm::vec2(max_tc.x, min_tc.y), tint);
    attribs.emplace_back(glm::vec2(max.x, max.y), glm::vec2(max_tc.x, max_tc.y), tint);

    attribs.emplace_back(glm::vec2(min.x, min.y), glm::vec2(min_tc.x, min_tc.y), tint);
    attribs.emplace_back(glm::vec2(max.x, max.y), glm::vec2(max_tc.x, max_tc.y), tint);
    attribs.emplace_back(glm::vec2(min.x, max.y), glm::vec2(min_tc.x, max_tc.y), tint);
}

DrawPicture::~DrawPicture() {

    if (attribs.empty()) return;

    //upload vertices to vertex_buffer:
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
    glBufferData(GL_ARRAY_BUFFER, attribs.size() * sizeof(attribs[0]), attribs.data(), GL_STREAM_DRAW); //upload attribs array
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //set color_texture_program as current program:
    glUseProgram(color_texture_program->program);

    //upload OBJECT_TO_CLIP to the proper uniform location:
    glUniformMatrix4fv(color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(to_clip));

    //use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
    glBindVertexArray(vertex_buffer_for_color_texture_program);

    //bind the sprite texture to location zero:
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pic.tex);

    //run the OpenGL pipeline:
    glDrawArrays(GL_TRIANGLES, 0, GLsizei(attribs.size()));

    //unbind the sprite texture:
    glBindTexture(GL_TEXTURE_2D, 0);

    //reset vertex array to none:
    glBindVertexArray(0);

    //reset current program to none:
    glUseProgram(0);

    //GL_ERRORS();
}