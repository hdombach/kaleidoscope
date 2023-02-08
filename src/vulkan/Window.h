#pragma once

#include <memory>
#include <GLFW/glfw3.h>

namespace vulkan {
	class Window;
	using SharedWindow = std::shared_ptr<Window>;
	using UniqueWindow = std::unique_ptr<Window>;

	struct WindowCreateInfo {
		uint32_t width;
		uint32_t height;
		const char * name;
	};

	class Window {
		public:
			static SharedWindow createShared(WindowCreateInfo &createInfo);
			static UniqueWindow createUnique(WindowCreateInfo &createInfo);
			GLFWwindow* operator*();
			GLFWwindow* raw();
			~Window();

		private:
			Window(WindowCreateInfo &createInfo);

			GLFWwindow * window_;
	};

	class WindowFactory {
		public:
			WindowFactory() = default;

			WindowFactory &defaultSetup();
			SharedWindow createShared();
			UniqueWindow createUnique();

		private:
			WindowCreateInfo createInfo{};
	};
}
