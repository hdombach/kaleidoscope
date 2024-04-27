#include "AppView.hpp"
#include "../App.hpp"
#include <imgui.h>

namespace ui {
	AppView::AppView(App& app):
		_app(app),
		_preview_viewport(app.scene().preview_texture()),
		_raytrace_viewport(app.scene().raytrace_texture())
	{}

	void AppView::show() {
		ImGui::Begin("Viewport");
		if (_showing_preview) {
			_preview_viewport.show();
		} else {
			_raytrace_viewport.show();
		}
		ImGui::End();

		ImGui::Begin("Settings");
		ImGui::Checkbox("Showing preview", &_showing_preview);
		ImGui::End();
	}
}
