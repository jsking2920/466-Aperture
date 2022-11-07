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
	} left, right, down, up, lmb, rmb, lctrl;
	glm::vec2 mouse_motion = glm::vec2(0, 0);

	// Local copy of the game scene
	Scene scene;

	// Audio
	std::shared_ptr< Sound::PlayingSample > music_loop;

	Player* player = nullptr;

	float day_length = 240.0f;// 4 minutes = 1 day, 10 seconds = 1 hour
	float time_of_day = 0.0f; // loops from 0 to day_length
	float sunrise = 60.0f; // sunrise at 6am
	float sunset = 210.0f; // sunset at 9pm

	// Text Rendering
	TextRenderer* display_text = nullptr;
	uint8_t display_font_size = 48;
	TextRenderer* barcode_text = nullptr;
	uint8_t barcode_font_size = 36;
};
