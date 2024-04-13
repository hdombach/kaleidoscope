#include <array>

#include <vulkan/vulkan_core.h>

#include "ColorMaterial.hpp"
#include "defs.hpp"
#include "shader.hpp"
#include "vertex.hpp"

namespace vulkan {
	util::Result<ColorMaterialPrevImpl, KError> ColorMaterialPrevImpl::create(
			PreviewRenderPass &render_pass,
			glm::vec3 color)
	{
		auto result = ColorMaterialPrevImpl(render_pass.descriptor_pool());
		result._color = color;

		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			auto buffer_res = MappedUniformObject::create();
			TRY(buffer_res);
			result._mapped_uniforms.push_back(std::move(buffer_res.value()));
		}
		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			auto color_buf_res = ColorUniformObject::create();
			TRY(color_buf_res);
			result._color_uniforms.push_back(std::move(color_buf_res.value()));
		}

		auto descriptor_templates = std::vector<DescriptorSetTemplate>();

		descriptor_templates.push_back(DescriptorSetTemplate::create_uniform(
					0,
					VK_SHADER_STAGE_VERTEX_BIT,
					result._mapped_uniforms));
		descriptor_templates.push_back(DescriptorSetTemplate::create_uniform(
					1, VK_SHADER_STAGE_FRAGMENT_BIT,
					result._color_uniforms));

		auto descriptor_sets = DescriptorSets::create(
				descriptor_templates,
				FRAMES_IN_FLIGHT,
				render_pass.descriptor_pool());
		TRY(descriptor_sets);
		result._descriptor_sets = std::move(descriptor_sets.value());

		/* Create pipeline */
		auto vert_shader = vulkan::Shader::fromEnvFile(
				"src/shaders/default_shader.vert.spv");
		auto frag_shader = vulkan::Shader::fromEnvFile(
				"src/shaders/color_shader.frag.spv");

		auto res = _create_pipeline(
				vert_shader, 
				frag_shader, 
				render_pass, 
				result._descriptor_sets.layout_ptr(), 
				&result._pipeline, 
				&result._pipeline_layout);
		TRY(res);


		return std::move(result);
	}

	ColorMaterialPrevImpl::ColorMaterialPrevImpl(ColorMaterialPrevImpl &&other):
		ColorMaterialPrevImpl(other._descriptor_sets.descriptor_pool())
	{
		_pipeline_layout = other._pipeline_layout;
		other._pipeline_layout = nullptr;

		_pipeline = other._pipeline;
		other._pipeline = nullptr;

		_descriptor_sets = std::move(other._descriptor_sets);

		_mapped_uniforms = std::move(other._mapped_uniforms);

		_color_uniforms = std::move(other._color_uniforms);

		_color = other._color;
	}

	ColorMaterialPrevImpl& ColorMaterialPrevImpl::operator=(ColorMaterialPrevImpl&& other)
	{
		_pipeline_layout = other._pipeline_layout;
		other._pipeline_layout = nullptr;

		_pipeline = other._pipeline;
		other._pipeline = nullptr;

		_descriptor_sets = std::move(other._descriptor_sets);

		_mapped_uniforms = std::move(other._mapped_uniforms);

		_color_uniforms = std::move(other._color_uniforms);

		_color = other._color;

		return *this;
	}

	ColorMaterialPrevImpl::~ColorMaterialPrevImpl() {
		if (_pipeline_layout) {
			vkDestroyPipelineLayout(
					Graphics::DEFAULT->device(), 
					_pipeline_layout, 
					nullptr);
			_pipeline_layout = nullptr;
		}

		if (_pipeline) {
			vkDestroyPipeline(
					Graphics::DEFAULT->device(), 
					_pipeline, 
					nullptr);
			_pipeline = nullptr;
		}
	}

	VkPipelineLayout ColorMaterialPrevImpl::pipeline_layout() {
		return _pipeline_layout;
	}

	VkPipeline ColorMaterialPrevImpl::pipeline() {
		return _pipeline;
	}

	VkDescriptorSet ColorMaterialPrevImpl::get_descriptor_set(uint32_t frame_index) {
		return _descriptor_sets.descriptor_set(frame_index);
	}

	void ColorMaterialPrevImpl::update_uniform(
			uint32_t frame_index,
			glm::vec3 position,
			glm::vec2 viewport_size)
	{
		static auto start_time = std::chrono::high_resolution_clock::now();
		auto current_time = std::chrono::high_resolution_clock::now();
		auto time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

		auto ubo = UniformBufferObject{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.model = glm::translate(ubo.model, position);
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), viewport_size.x / (float) viewport_size.y, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		_mapped_uniforms[frame_index].set_value(ubo);

		auto color = ColorBufferObject{_color};
		color.color.y += 0.2 + sin(time * 10) * 0.2;
		_color_uniforms[frame_index].set_value(color);
	}


	ColorMaterialPrevImpl::ColorMaterialPrevImpl(DescriptorPool &descriptor_pool):
		_descriptor_sets(descriptor_pool)
	{ }

	ColorMaterial::ColorMaterial(glm::vec3 color):
		_color(color)
	{ }

	util::Result<void, KError> ColorMaterial::add_preview(
			PreviewRenderPass &preview_render_pass)
	{
		auto res = ColorMaterialPrevImpl::create(preview_render_pass, _color);
		if (res) {
			_preview_impl = std::move(res.value());
			return {};
		} else {
			return res.error();
		}
	}

	ColorMaterialPrevImpl *ColorMaterial::preview_impl() {
		if (_preview_impl) {
			return &_preview_impl.value();
		} else {
			return nullptr;
		}
	}

	ColorMaterialPrevImpl const *ColorMaterial::preview_impl() const {
		if (_preview_impl) {
			return &_preview_impl.value();
		} else {
			return nullptr;
		}
	}

}
