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
	// Which call the appropriate update and draw functions based on cur_state for state specific things
	void menu_update(float elapsed);
	void menu_draw_ui(glm::uvec2 const& drawable_size);
	void playing_update(float elapsed);
	void playing_draw_ui(glm::uvec2 const& drawable_size);
	void journal_update(float elapsed);
	void journal_draw_ui(glm::uvec2 const& drawable_size);
	void night_update(float elapsed);
	void night_draw_ui(glm::uvec2 const& drawable_size);

	//----- Game State -----
	enum GameState {
		menu, playing, journal, night
	};
	GameState cur_state = menu;

	// Input tracking
	struct Button {
		uint8_t downs = 0;
		bool pressed = false;
	} left, right, down, up, lmb, rmb, lctrl, tab, enter;
	struct Mouse {
		glm::vec2 mouse_motion = glm::vec2(0, 0);
		uint8_t moves = 0;
		int32_t wheel_y = 0;
		bool scrolled = false;
	} mouse;

	// Player Settings
	// TODO: should make a config file for player to be able to edit for these
	float mouse_sensitivity = 0.5f;

	// Local copy of the game scene
	Scene scene;

	// Audio
    std::map<std::string, Sound::Sample> sample_map;
	std::shared_ptr< Sound::PlayingSample > music_loop;

	// Player
	Player* player = nullptr;
	WalkPoint cur_spawn;

	// Day/Night Cycle Stuff
	float time_of_day = 70.0f; // loops from 0 to day_length, starts at 7 AM (should be equal to start_day_time at initialization)

	float day_length = 240.0f;// 4 minutes = 1 day, 10 seconds = 1 hour
	float start_day_time = 70.0; // Time player starts every day at (7:00am)
	float end_day_time = 230.0f; // Time player ends of everyday (11:00pm)

	float time_scale = 4.0f; // multiplier for speed of time (starts at 4x for menu visuals)

	float sunrise = 75.0f; // sunrise at 7:30am
	float sunset = 210.0f; // sunset at 9pm

	glm::vec3 day_sky_color = glm::vec3(0.5f, 1.0f, 1.0f);
	glm::vec3 night_sky_color = glm::vec3(0.f, 0.02f, 0.1f);
	glm::vec3 sunset_sky_color = glm::vec3(0.5f, 0.3f, 0.1f);
    glm::vec3 day_ambient_color = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 night_ambient_color = glm::vec3(0.1f, 0.1f, 0.3f);
    glm::vec3 sunset_ambient_color = glm::vec3(0.9f, 0.6f, 0.4f);

	// Text Rendering
	TextRenderer* display_text = nullptr;
	uint8_t display_font_size = 96;
	TextRenderer* body_text = nullptr;
	uint8_t body_font_size = 48;
	TextRenderer* barcode_text = nullptr;
	uint8_t barcode_font_size = 36;
	
	// UI
	float score_text_popup_timer = 0.0f;
	float score_text_popup_duration = 2.0f;
	bool score_text_is_showing = false;
};
