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

	// Reference to the creature
	std::vector< Creature* > creatures;

	// Audio
	std::shared_ptr< Sound::PlayingSample > music_loop;

	Player player;
    std::list<Picture> pictures;

	// Text Rendering
	TextRenderer* ui_text = nullptr;
	uint8_t ui_font_size = 34;
};
