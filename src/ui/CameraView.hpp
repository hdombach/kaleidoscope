#pragma once

namespace types {
	class Camera;
};

namespace ui {
	class CameraView {
		public:
			CameraView(types::Camera &camera);
			void show();
			static void show(types::Camera &camera);

		private:
			types::Camera &_camera;
	};
}
