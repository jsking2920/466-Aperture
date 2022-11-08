#include "Picture.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"

#include <algorithm>
#include <utility>
#include <filesystem>

Picture::Picture(PictureStatistics stats) : dimensions(stats.dimensions), data(std::move(stats.data)) {
    if (stats.frag_counts.empty()) {
        title = "Pure Emptiness";
        score_elements.emplace_back("Relatable", 500);
        score_elements.emplace_back("Deep", 500);
        return;
    }

    auto sort_by_frag_count = [&](std::pair<Scene::Drawable &, GLuint> a, std::pair<Scene::Drawable &, GLuint> b) {
        return a.second > b.second;
    };
    stats.frag_counts.sort(sort_by_frag_count);

    //TODO: improve subject selection process
    Scene::Drawable &subject = stats.frag_counts.front().first;
    unsigned int subject_frag_count = stats.frag_counts.front().second;

    //Add points for bigness
    score_elements.emplace_back("Bigness", subject_frag_count/1000);

    //Add bonus points for additional subjects
    std::for_each(std::next(stats.frag_counts.begin()), stats.frag_counts.end(), [&](std::pair<Scene::Drawable &, GLuint> creature) {
        score_elements.emplace_back("Bonus " + creature.first.transform->name, 100);
    });

    //Magnificence
    score_elements.emplace_back("Magnificence", 200);

    //TODO: make name unique for file saving purposes
    title = "Magnificent " + subject.transform->name;
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
    uint8_t* png_data = new uint8_t[4 * dimensions.x * dimensions.y];
    for (uint32_t i = 0; i < dimensions.x * dimensions.y; i++) {
        png_data[i * 4] = (uint8_t)round(data[i * 3] * 255);
        png_data[i * 4 + 1] = (uint8_t)round(data[i * 3 + 1] * 255);
        png_data[i * 4 + 2] = (uint8_t)round(data[i * 3 + 2] * 255);
        png_data[i * 4 + 3] = 255;
    }
    // create album folder if it doesn't exist
    if (!std::filesystem::exists(data_path("PhotoAlbum/"))) {
        std::filesystem::create_directory(data_path("PhotoAlbum/"));
    }
    save_png(data_path("PhotoAlbum/" + title + ".png"), dimensions,
             reinterpret_cast<const glm::u8vec4*>(png_data), LowerLeftOrigin);
    delete[] png_data;
}
