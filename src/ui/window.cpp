#include "window.h"
#include "defs.h"
#include "format.h"
#include "graphics.h"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "log.h"
#include "mainRenderPipeline.h"
#include "textureView.h"
#include "vulkan/vulkan_core.h"
#include <memory>
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <vector>

namespace ui {
	Window::Window(vulkan::Graphics const &graphics):
		graphics_(graphics),
		mainRenderPipeline_(std::make_unique<vulkan::MainRenderPipeline>(graphics_, VkExtent2D{1000, 1000})),
		viewport_(*mainRenderPipeline_)
	{
		tempLoadModel_();
	}
	Window::~Window() {
		mainRenderPipeline_.reset();
	}

	void Window::show() {
		mainRenderPipeline_->submit();
		auto imguiViewport = ImGui::GetMainViewport();
		//ImGui::SetNextWindowPos(imguiViewport->WorkPos);
		//ImGui::SetNextWindowSize(imguiViewport->WorkSize);

		ImGui::Begin("Hello World Example");
		ImGui::Text("Hello World");

		viewport_.show();

		ImGui::End();
	}

	void Window::tempLoadModel_() {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;
		std::vector<vulkan::Vertex> vertices;
		std::vector<uint32_t> indices;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, vulkan::MODEL_PATH.c_str())) {
			util::log_error(warn + err);
		}

		std::unordered_map<vulkan::Vertex, uint32_t> uniqueVertices{};

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				vulkan::Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2],
				};

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0 - attrib.texcoords[2 * index.texcoord_index + 1],
				};

				vertex.color = {1.0f, 1.0f, 1.0f};

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
		}

		mainRenderPipeline_->loadVertices(vertices, indices);
	}
}
