#include "CameraView.hpp"
#include "../types/Camera.hpp"
#include <array>
#include <imgui.h>

namespace ui {
	CameraView::CameraView(types::Camera &camera):
		_camera(camera)
	{}

	void CameraView::show() {
		show(_camera);
	}

	void CameraView::show(types::Camera &camera) {
		auto pos = camera.get_position_array();

		ImGui::Text("Camera");
		ImGui::DragFloat3("Position", pos.data(), 0.01f);

		ImGui::DragFloat("fovy", &camera.fovy);

		camera.set_position(pos);
	}
}
