#pragma once

#include "Texture.hpp"

namespace vulkan {
	class Scene;

	class SceneTexture: public Texture {
		public:
			SceneTexture(uint32_t id, Scene &scene);
			VkDescriptorSet imgui_descriptor_set() override;
			ImageView const &image_view() const override;
			uint32_t id() const override;
			std::string const &name() const override { return _name; }
			void set_name(std::string const &name) override { _name = name; }

			bool is_resizable() const override {
				return true;
			}

			void resize(VkExtent2D size) override;
		private:
			Scene &_scene;
			uint32_t _id;
			std::string _name;
	};
}
