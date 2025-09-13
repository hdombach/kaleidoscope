#include "CameraView.hpp"
#include "types/Camera.hpp"
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
		auto rotation = camera.get_rotation_array();
		auto size = std::array<int, 2>{camera.width(), camera.height()};
		auto fovy = camera.fovy();
		auto de_iterations = camera.de_iterations();
		auto de_small_step = camera.de_small_step();

		ImGui::Text("Camera");
		ImGui::DragFloat3("Position", pos.data(), 0.01f);
		ImGui::DragFloat3("Rotation", rotation.data(), 0.2f);
		ImGui::DragInt2("Size", size.data());

		ImGui::DragFloat("fovy", &fovy);
		ImGui::DragInt("DE Iterations", &de_iterations);
		ImGui::DragFloat("DE Small Step", &de_small_step, 0.1, 0);

		camera.set_position(pos);
		camera.set_rotation(rotation);
		camera.set_width(size[0]);
		camera.set_height(size[1]);
	}
}
