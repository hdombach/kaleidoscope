#pragma once

#include "textureView.hpp"
#include <glm/fwd.hpp>

class App;
namespace ui {
	class AppView {
		public:
			AppView(App &app);
			void show();

			bool showing_preview() { return _showing_preview; }

		private:
			App &_app;
			TextureView _scene_viewport;

			bool _showing_preview;
			glm::vec2 _previous_mouse_pos;

			static glm::vec3 _get_camera_movement();
	};
}
