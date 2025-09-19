#pragma once

#include <stddef.h>

#include "util/UIDList.hpp"
#include "types/Material.hpp"
#include "types/ResourceManager.hpp"
#include "codegen/TemplObj.hpp"
#include "vulkan/Scene.hpp"
#include "vulkan/StaticBuffer.hpp"

/**
 * @file
 * Common functions accross preview pass and raytrace pass
 */

namespace vulkan {
	using MaterialContainer = types::ResourceManager::MaterialContainer;

	size_t max_material_range(MaterialContainer const &materials);

	cg::TemplObj material_templobj(
		uint32_t id,
		MaterialContainer const &materials
	);

	util::Result<StaticBuffer, KError> create_material_buffer(Scene &scene);
}
