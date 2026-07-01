vulkan_sources = [
	'DescriptorPool.cpp',
	'DescriptorSet.cpp',
	'Fence.cpp',
	'Image.cpp',
	'Sampler.cpp',
	'Scene.cpp',
	'SceneTexture.cpp',
	'Semaphore.cpp',
	'Shader.cpp',
	'StaticBuffer.cpp',
	'StaticTexture.cpp',
	'UIRenderPipeline.cpp',
	'graphics.cpp',
	'PassUtil.cpp',
	'Error.cpp',
	'DescAttachment.cpp',
	'FrameAttachment.cpp',
	'RenderPass.cpp',
	'Pipeline.cpp',

	'prev_pass/InstancedPass.cpp',
	'prev_pass/InstancedPassMesh.cpp',
	'prev_pass/InstancedPassNode.cpp',

	'ray_pass/RayPass.cpp',
	'ray_pass/RayPassMaterial.cpp',
	'ray_pass/RayPassMesh.cpp',
	'ray_pass/RayPassNode.cpp',

	meson.build_root() + '/imgui_impl_vulkan.cpp',
	meson.build_root() + '/imgui_impl_glfw.cpp',
]

vulkan_deps = [
	vulkan,
	stb,
	glm,
	glfw3,
	imgui,
	shaderc,
	tinyobjloader,
]

vulkan_wrap = static_library(
	'vulkan_wrap',
	vulkan_sources,
  include_directories: [src_include],
	dependencies: vulkan_deps,
  link_with: util
)
