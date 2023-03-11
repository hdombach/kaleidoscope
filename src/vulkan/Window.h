#pragma once

#include <functional>
#include <memory>
#include <GLFW/glfw3.h>

namespace vulkan {
	class Window;
	class Graphics;
	using SharedWindow = std::shared_ptr<Window>;

	struct WindowDeleter {
		void operator()(GLFWwindow** window);
	};

	class Window: public std::unique_ptr<GLFWwindow*, WindowDeleter> {
		public:
			Window() = default;

			Window(const char *name, uint32_t width=800, uint32_t height=600);

			GLFWwindow* raw();

			void set_graphics(Graphics * graphics);
		private:
			static void framebufferResizeCallback_(GLFWwindow*window, int width, int height);

			Graphics * graphics_;
	};
}
