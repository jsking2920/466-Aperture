#include "Picture.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"

#include <algorithm>
#include <utility>
#include <filesystem>
#include <math.h>

const std::string Picture::adjectives[] = { "Majestic", "Beautiful", "Vibrant", "Grand", "Impressive", "Awe-inspiring", "Sublime", "Resplendent", "Striking", "Marvelous", "Alluring", "Gorgeous", "Glamorous", "Divine", "Exquisite", "Stunning", "Beauteous", "Comely", "Pulchritudinous", "Graceful", "Dazzling", "Lovely", "Superb", "Resplendent", "Radiant", "Well-formed", "Great"};

Picture::Picture(PictureInfo &stats) : dimensions(stats.dimensions), data(stats.data) {
    if (stats.frag_counts.empty()) {
        title = "Pure Emptiness";
        score_elements.emplace_back("Relatable", (uint32_t)500);
        score_elements.emplace_back("Deep", (uint32_t)500);
        return;
    }

    if (stats.creatures_in_frame.empty()) {
        //TODO: once we add in some points for nature, make this better
        title = "Beautiful Nature";
        score_elements.emplace_back("So peaceful!", (uint32_t)2000);
        return;
    }

    PictureCreatureInfo subject_info = stats.creatures_in_frame.front();

    //grade subject
    {   
        //Magnificence
        score_elements.emplace_back(subject_info.creature->name, subject_info.creature->score);
        auto result = score_creature(subject_info, stats);
        score_elements.insert(score_elements.end(), result.begin(), result.end()); //from https://stackoverflow.com/q/1449703
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
        float lo = (float)std::cos( worst_degrees_deviated * M_PI / 180.f);
        float hi = (float)std::cos(best_degrees_deviated * M_PI / 180.f);
        float clamped_dot = std::clamp(dot, lo, hi);
        //normalized to be between 0 and 1
        float normalized_dot =  (clamped_dot - lo) / (hi - lo);
        if(normalized_dot > 0.01) {
            result.emplace_back("Angle", (uint32_t) (std::pow(normalized_dot, exponent) * 3000.0f));
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
