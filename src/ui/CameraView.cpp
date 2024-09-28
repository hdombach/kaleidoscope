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
		auto rotation = camera.get_euler_rotation();
		auto size = std::array<int, 2>{camera.width, camera.height};

		ImGui::Text("Camera");
		ImGui::DragFloat3("Position", pos.data(), 0.01f);
		ImGui::DragFloat3("Rotation", rotation.data(), 0.2f);
		ImGui::DragInt2("Size", size.data());

		ImGui::DragFloat("fovy", &camera.fovy);

		camera.set_position(pos);
		camera.set_euler_rotation(rotation);
		camera.width = size[0];
		camera.height = size[1];
	}
}
