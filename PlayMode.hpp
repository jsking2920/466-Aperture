#include "Mode.hpp"

#include "Scene.hpp"
#include "WalkMesh.hpp"
#include "Sound.hpp"
#include "TextRenderer.hpp"
#include "Player.hpp"
#include "Picture.hpp"
#include "GameObjects.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <map>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	// Functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- Game State -----

	// Input tracking
	struct Button {
		uint8_t downs = 0;
		bool pressed = false;
	} left, right, down, up, lmb, rmb, lctrl, tab;
	glm::vec2 mouse_motion = glm::vec2(0, 0);

	// Local copy of the game scene
	Scene scene;

	// Audio
    std::map<std::string, Sound::Sample> sample_map;
	std::shared_ptr< Sound::PlayingSample > music_loop;

	// Player
	Player* player = nullptr;

	// Day/Night Cycle Stuff
	float day_length = 240.0f;// 4 minutes = 1 day, 10 seconds = 1 hour
	float time_of_day = 65.0f; // loops from 0 to day_length, starts at 6:30 AM
	float sunrise = 60.0f; // sunrise at 6am
	float sunset = 200.0f; // sunset at 8pm

	glm::vec3 day_sky_color = glm::vec3(0.5f, 1.0f, 1.0f);
	glm::vec3 night_sky_color = glm::vec3(0.f, 0.02f, 0.1f);
	glm::vec3 sunset_sky_color = glm::vec3(0.5f, 0.3f, 0.1f);
    glm::vec3 day_ambient_color = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 night_ambient_color = glm::vec3(0.1f, 0.1f, 0.3f);
    glm::vec3 sunset_ambient_color = glm::vec3(0.9f, 0.6f, 0.4f);

	// Text Rendering
	TextRenderer* display_text = nullptr;
	uint8_t display_font_size = 48;
	TextRenderer* barcode_text = nullptr;
	uint8_t barcode_font_size = 36;
	
	// UI
	float score_text_popup_timer = 0.0f;
	float score_text_popup_duration = 2.0f;
	bool score_text_is_showing = false;
};
