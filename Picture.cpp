#include "Picture.hpp"

Picture::Picture() {
    //TODO: Pass in texture to constructor and save

    //TODO: Export as png?
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
