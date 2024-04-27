#pragma once

#include "textureView.hpp"

class App;
namespace ui {
	class AppView {
		public:
			AppView(App &app);
			void show();

			bool showing_preview() { return _showing_preview; }

		private:
			App &_app;
			TextureView _preview_viewport;
			TextureView _raytrace_viewport;

			bool _showing_preview;

	};
}
