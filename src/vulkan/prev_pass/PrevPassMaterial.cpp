#include <array>
#include <source_location>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "PrevPassMaterial.hpp"
#include "PrevPass.hpp"
#include "util/BaseError.hpp"
#include "util/log.hpp"
#include "vulkan/DescAttachment.hpp"
#include "vulkan/Vertex.hpp"
#include "vulkan/Pipeline.hpp"
#include "util/file.hpp"
#include "util/Util.hpp"
#include "types/Material.hpp"
#include "codegen/TemplObj.hpp"
#include "codegen/TemplGen.hpp"

namespace vulkan {
	PrevPassMaterial::Error PrevPassMaterial::vulkan_err(
		VkResult vk_error,
		const char *msg
	) {
		return Error(ErrorType::VULKAN_ERR, msg, VkError(vk_error));
	}

	util::Result<PrevPassMaterial::Ptr, PrevPassMaterial::Error> PrevPassMaterial::create(
			Scene &scene,
			PrevPass &preview_pass,
			const types::Material *material)
	{
		auto result = Ptr(new PrevPassMaterial());

		if (material == nullptr) {
			return Error(ErrorType::INVALID_ARG, "Cannot create null material");
		}

		result->_material = material;
		result->_prev_pass = &preview_pass;
		result->_pipeline_ready = false;

		auto r = result.get();

		if (auto err = r->_create().move_or()) {
			return Error(ErrorType::MISC, "Could not finish main create function", *err);
		}

		return std::move(result);
	}

	PrevPassMaterial::PrevPassMaterial(PrevPassMaterial &&other) {
		_material = other._material;
		other._material = nullptr;

		_prev_pass = other._prev_pass;
		other._prev_pass = nullptr;

		_pipeline = std::move(other._pipeline);
	}

	PrevPassMaterial& PrevPassMaterial::operator=(PrevPassMaterial &&other) {
		destroy();

		_material = other._material;
		other._material = nullptr;

		_prev_pass = other._prev_pass;
		other._prev_pass = nullptr;

		_pipeline = std::move(other._pipeline);

		return *this;
	}

	void PrevPassMaterial::destroy() {
		_material = nullptr;
		_prev_pass = nullptr;
		_pipeline.destroy();
	}

	PrevPassMaterial::~PrevPassMaterial() {
		destroy();
	}

	bool PrevPassMaterial::exists() const {
		return _pipeline.has_value();
	}

	uint32_t PrevPassMaterial::id() const {
		return _material->id();
	}

	VkPipeline PrevPassMaterial::pipeline() {
		return _pipeline_ready ? _pipeline.pipeline() : nullptr;
	}

	VkPipelineLayout PrevPassMaterial::pipeline_layout() {
		return _pipeline_ready ? _pipeline.pipeline_layout() : nullptr;
	}

	void PrevPassMaterial::_codegen(
			std::string &frag_source,
			std::string &vert_source,
			const types::Material *material,
			std::vector<std::string> &textures)
	{
		frag_source = util::readEnvFile("assets/shaders/preview_material.frag.cg");

		vert_source = util::readEnvFile("assets/shaders/preview_material.vert.cg");
		auto vert_args = cg::TemplObj{
			{"global_declarations", GlobalPrevPassUniform::declaration_content},
			{"material_declarations", material->resources().templ_declarations()}
		};
		auto start = log_start_timer();
		vert_source = cg::TemplGen::codegen(vert_source, vert_args.dict().value(), "preview_material.vert.cg").value();
		log_info() << "codegenerating preview_material.vert.cg took " << start << std::endl;

		auto templ_textures = cg::TemplList();
		for (auto &t : textures) {
			templ_textures.push_back(t);
		}

		auto frag_args = cg::TemplObj{
			{"global_declarations", GlobalPrevPassUniform::declaration_content},
			{"material_declarations", material->resources().templ_declarations()},
			{"textures", templ_textures},
			{"frag_source", material->frag_shader_src()}
		};
		start = log_start_timer();
		frag_source = cg::TemplGen::codegen(frag_source, frag_args.dict().value(), "preview_material.frag.cg").value();
		log_info() << "codegenerating preview_material.frag.cg took " << start << std::endl;

		log_trace() << "vert codegen:\n" << util::add_strnum(vert_source) << std::endl;
		log_trace() << "frag codegen:\n" << util::add_strnum(frag_source) << std::endl;
	}

	util::Result<void, PrevPassMaterial::Error> PrevPassMaterial::_create() {
		/* code gen vertex code */
		auto vert_source = std::string();
		auto frag_source = std::string();
		auto texture_names = std::vector<std::string>();
		for (auto resource : _material->resources().get()) {
			if (resource->type() == types::ShaderResource::Type::Texture) {
				texture_names.push_back(resource->name());
			}
		}
		_codegen(frag_source, vert_source, _material, texture_names);

		vulkan::Shader vert_shader, frag_shader;
		if (auto shader = vulkan::Shader::from_source_code(vert_source, Shader::Type::Vertex)) {
			vert_shader = shader.move_value();
		} else {
			return Error(ErrorType::MISC, "Could not create vert shader", shader.error());
		}

		if (auto shader = vulkan::Shader::from_source_code(frag_source, Shader::Type::Fragment)) {
			frag_shader = shader.move_value();
		} else {
			return Error(ErrorType::MISC, "Could not create frag shader", shader.error());
		}

		auto attachments = Pipeline::Attachments{
			{
				DescAttachment::create_uniform(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
			},
			{
				DescAttachment::create_uniform(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
			}
		};

		if (texture_names.size() > 0) {
			attachments[1].push_back(DescAttachment::create_images(VK_SHADER_STAGE_FRAGMENT_BIT, texture_names.size()));
		}

		if (auto err = Pipeline::create_graphics(
				vert_shader,
				frag_shader,
				_prev_pass->render_pass(),
				attachments
		).move_or(_pipeline)) {
			return Error(ErrorType::MISC, "Could not create pipeline", err.value());
		}

		_pipeline_ready = true;

		return {};
	}
}

template<>
	const char *vulkan::PrevPassMaterial::Error::type_str(vulkan::PrevPassMaterial::ErrorType t) {
		switch (t) {
			case vulkan::PrevPassMaterial::ErrorType::INVALID_ARG:
				return "PrevPassMaterial.INVALID_ARG";
			case vulkan::PrevPassMaterial::ErrorType::VULKAN_ERR:
				return "PrevPassMaterial.VULKAN_ERR";
			case vulkan::PrevPassMaterial::ErrorType::MISC:
				return "PrevPassMaterial.MISC";
		}
	}
