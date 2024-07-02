#include <chrono>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/fwd.hpp>
#include <optional>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>
#include <glm/vector_relational.hpp>

#include "TextureMaterial.hpp"
#include "DescriptorSet.hpp"
#include "defs.hpp"
#include "graphics.hpp"
#include "PreviewRenderPass.hpp"
#include "Shader.hpp"
#include "../util/file.hpp"

namespace vulkan {
	util::Result<TextureMaterialPrevImpl, KError> TextureMaterialPrevImpl::create(
			PreviewRenderPass &render_pass,
			Texture *texture)
	{
		auto result = TextureMaterialPrevImpl(render_pass.descriptor_pool());
		result._texture = texture;

		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			auto buffer_res = MappedUniform::create();
			TRY(buffer_res);
			result._mapped_uniforms.push_back(std::move(buffer_res.value()));
		}

		auto descriptor_templates = std::vector<DescriptorSetTemplate>();

		descriptor_templates.push_back(DescriptorSetTemplate::create_uniform(
					0, 
					VK_SHADER_STAGE_VERTEX_BIT, 
					result._mapped_uniforms));
		descriptor_templates.push_back(DescriptorSetTemplate::create_image(
					1,
					VK_SHADER_STAGE_FRAGMENT_BIT,
					texture->image_view()));

		auto descriptor_sets = DescriptorSets::create(
				descriptor_templates,
				FRAMES_IN_FLIGHT,
				render_pass.descriptor_pool());

		TRY(descriptor_sets);
		result._descriptor_sets = std::move(descriptor_sets.value());

		/* Create pipeline */
		auto vert_shader = vulkan::Shader::from_env_file(
				"src/shaders/default_shader.vert.spv");
		auto frag_shader = vulkan::Shader::from_source_code(
				util::readEnvFile("assets/default_shader.frag"));
		TRY(frag_shader);
		/*auto frag_shader = vulkan::Shader::from_env_file(
				"src/shaders/default_shader.frag.spv");*/

		auto descriptor_layouts = std::vector<VkDescriptorSetLayout>{
			render_pass.global_descriptor_set_layout(),
			result._descriptor_sets.layout(),
		};
		auto res = _create_pipeline(
				vert_shader, 
				frag_shader.value(), 
				render_pass, 
				descriptor_layouts, 
				&result._pipeline, 
				&result._pipeline_layout);
		TRY(res);

		return std::move(result);
	}

	TextureMaterialPrevImpl::TextureMaterialPrevImpl(TextureMaterialPrevImpl &&other):
		vulkan::TextureMaterialPrevImpl(other._descriptor_sets.descriptor_pool())
	{
		_texture = other._texture;

		_pipeline_layout = other._pipeline_layout;
		other._pipeline_layout = nullptr;

		_pipeline = other._pipeline;
		other._pipeline = nullptr;

		_descriptor_sets = std::move(other._descriptor_sets);

		_mapped_uniforms = std::move(other._mapped_uniforms);
	}

	TextureMaterialPrevImpl& TextureMaterialPrevImpl::operator=(TextureMaterialPrevImpl&& other) {
			_texture = other._texture;

		_pipeline_layout = other._pipeline_layout;
		other._pipeline_layout = nullptr;

		_pipeline = other._pipeline;
		other._pipeline = nullptr;

		_descriptor_sets = std::move(other._descriptor_sets);

		_mapped_uniforms = std::move(other._mapped_uniforms);

		return *this;
	}

	TextureMaterialPrevImpl::~TextureMaterialPrevImpl() {
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

	VkPipelineLayout TextureMaterialPrevImpl::pipeline_layout() {
		return _pipeline_layout;
	}

	VkPipeline TextureMaterialPrevImpl::pipeline() {
		return _pipeline;
	}

	VkDescriptorSet TextureMaterialPrevImpl::get_descriptor_set(uint32_t frame_index) {
		return _descriptor_sets.descriptor_set(frame_index);
	}

	void TextureMaterialPrevImpl::update_uniform(
			uint32_t frame_index,
			glm::vec3 position,
			glm::vec2 viewport_size)
	{
		static auto start_time = std::chrono::high_resolution_clock::now();
		auto current_time = std::chrono::high_resolution_clock::now();
		auto time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

		auto model_mat = glm::translate(glm::mat4(1.0), position + glm::vec3(0, 0, 0.2) * sin(time * 5));

		auto uniform_buffer = UniformBuffer{};
		uniform_buffer.object_transformation = model_mat;

		_mapped_uniforms[frame_index].set_value(uniform_buffer);
	}

	TextureMaterialPrevImpl::TextureMaterialPrevImpl(DescriptorPool &descriptor_pool):
		_texture(nullptr),
		_pipeline_layout(nullptr),
		_pipeline(nullptr),
		_descriptor_sets()
	{}

	TextureMaterial::TextureMaterial(uint32_t id, Texture* texture):
	_texture(texture), _id(id)
	{
		_uniform.get()->object_transformation = glm::mat4(1.0);
		_resources.push_back(types::ShaderResource::create_uniform("object_uniform", _uniform));
		_resources.push_back(types::ShaderResource::create_image("texSampler", texture->image_view()));
	}

	util::Result<void, KError> TextureMaterial::add_preview(
			PreviewRenderPass &preview_render_pass)
	{
		auto res = TextureMaterialPrevImpl::create(preview_render_pass, _texture);
		if (res) {
			_preview_impl = std::move(res.value());
			return {};
		} else {
			return res.error();
		}
	}

	MaterialPreviewImpl *TextureMaterial::preview_impl() {
		if (_preview_impl) {
			return &_preview_impl.value();
		} else {
			return nullptr;
		}
	}

	MaterialPreviewImpl const *TextureMaterial::preview_impl() const {
		if (_preview_impl) {
			return &_preview_impl.value();
		} else {
			return nullptr;
		}
	}
}
