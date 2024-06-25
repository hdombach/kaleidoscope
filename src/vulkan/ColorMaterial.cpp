#include <vector>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <vulkan/vulkan_core.h>

#include "ColorMaterial.hpp"
#include "defs.hpp"
#include "Shader.hpp"

namespace vulkan {
	util::Result<ColorMaterialPrevImpl, KError> ColorMaterialPrevImpl::create(
			PreviewRenderPass &render_pass,
			glm::vec3 color)
	{
		auto result = ColorMaterialPrevImpl();
		result._color = color;

		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			auto buffer_res = MappedUniform::create();
			TRY(buffer_res);
			result._mapped_uniforms.push_back(std::move(buffer_res.value()));
		}
		auto descriptor_templates = std::vector<DescriptorSetTemplate>();

		descriptor_templates.push_back(DescriptorSetTemplate::create_uniform(
					0,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					result._mapped_uniforms));

		auto descriptor_sets = DescriptorSets::create(
				descriptor_templates,
				FRAMES_IN_FLIGHT,
				render_pass.descriptor_pool());
		TRY(descriptor_sets);
		result._descriptor_sets = std::move(descriptor_sets.value());

		/* Create pipeline */
		auto vert_shader = vulkan::Shader::from_env_file(
				"src/shaders/default_shader.vert.spv");
		auto frag_shader = vulkan::Shader::from_env_file(
				"src/shaders/color_shader.frag.spv");

		auto descriptor_layouts = std::vector<VkDescriptorSetLayout>{
			render_pass.global_descriptor_set_layout(),
			result._descriptor_sets.layout(),
		};
		auto res = _create_pipeline(
				vert_shader, 
				frag_shader, 
				render_pass, 
				descriptor_layouts, 
				&result._pipeline, 
				&result._pipeline_layout);
		TRY(res);


		return std::move(result);
	}

	ColorMaterialPrevImpl::ColorMaterialPrevImpl(ColorMaterialPrevImpl &&other):
		ColorMaterialPrevImpl()
	{
		_pipeline_layout = other._pipeline_layout;
		other._pipeline_layout = nullptr;

		_pipeline = other._pipeline;
		other._pipeline = nullptr;

		_descriptor_sets = std::move(other._descriptor_sets);

		_mapped_uniforms = std::move(other._mapped_uniforms);

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

		_color = other._color;

		return *this;
	}

	ColorMaterialPrevImpl::~ColorMaterialPrevImpl() {
		_mapped_uniforms.clear();
		_descriptor_sets.clear();
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

		auto model_mat = glm::translate(glm::mat4(1.0f), position);
		model_mat = glm::rotate(model_mat, time, glm::vec3(0, 0, 1));

		auto uniform_buffer = UniformBuffer{};
		uniform_buffer.object_transformation = model_mat;
		uniform_buffer.color = _color;
		uniform_buffer.color.y += 0.2 + sin(time * 10) * 0.2;

		_mapped_uniforms[frame_index].set_value(uniform_buffer);
	}


	ColorMaterial::ColorMaterial(uint32_t id, glm::vec3 color):
		_id(id),
		_color(color)
	{
		_resources.push_back(types::ShaderResource::create_uniform("uniform_buffer", _uniform));
	}

	std::vector<types::ShaderResource> const &ColorMaterial::resources() const {
		return _resources;
	}	

	uint32_t ColorMaterial::id() const {
		return _id;
	}

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
