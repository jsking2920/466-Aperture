#include "Mode.hpp"

#include "Scene.hpp"
#include "WalkMesh.hpp"
#include "Sound.hpp"

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
		uint8_t pressed = 0;
	} left, right, down, up;

	// Local copy of the game scene
	Scene scene;

	// Audio
	std::shared_ptr< Sound::PlayingSample > music_loop;

	// Player
	struct Player {
		WalkPoint at;
		// Transform is at player's feet
		Scene::Transform *transform = nullptr;
		// Camera is at player's head
		Scene::Camera *camera = nullptr;
	} player;
};
