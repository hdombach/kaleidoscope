#pragma once

#include <string>
#include <imgui.h>

namespace ui {
	//Copied from imgui_stdlib because it's not appearing correctly
	bool  InputText(
			const char* label,
			std::string* str,
			ImGuiInputTextFlags flags = 0,
			ImGuiInputTextCallback callback = NULL,
			void* user_data = NULL);
}
