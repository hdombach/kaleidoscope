#include "AppView.hpp"
#include "../App.hpp"
#include <imgui.h>

namespace ui {
	AppView::AppView(App& app):
		_app(app),
		_scene_viewport(app.scene()),
		_showing_preview(true)
	{}

	void AppView::show() {
		ImGui::Begin("Viewport");
		_scene_viewport.show();
		ImGui::End();

		ImGui::Begin("Settings");
		ImGui::Checkbox("Showing preview", &_showing_preview);
		ImGui::End();

		_app.scene().set_is_preview(_showing_preview);
	}
}
