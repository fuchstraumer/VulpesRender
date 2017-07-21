#pragma once
#ifndef VULPES_VK_VERTEX_TYPES_H
#define VULPES_VK_VERTEX_TYPES_H

#include "stdafx.h"
#include "objects\ForwardDecl.h"


namespace vulpes {

	struct VertexBase2D {
		glm::vec2 Position;
		glm::vec3 Color;
		glm::vec2 UV;

		constexpr static VkVertexInputBindingDescription BindingDescr() noexcept {
			const VkVertexInputBindingDescription binding_descr{
				0,
				static_cast<uint32_t>(sizeof(VertexBase2D)),
				VK_VERTEX_INPUT_RATE_VERTEX,
			};
			return binding_descr;
		}

		constexpr static std::array<const VkVertexInputAttributeDescription, 3> AttrDescr() noexcept {
			constexpr std::array< const VkVertexInputAttributeDescription, 3> result = {
				VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<uint32_t>(offsetof(VertexBase2D, Position)) },
				VkVertexInputAttributeDescription{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(VertexBase2D, Color)) },
				VkVertexInputAttributeDescription{ 2, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<uint32_t>(offsetof(VertexBase2D, UV)) },
			};
			return result;
		}
	};

	struct Vertex3D {
		glm::vec3 Position;
		glm::vec2 UV;
		constexpr static VkVertexInputBindingDescription BindingDescr() noexcept {
			const VkVertexInputBindingDescription binding_descr{
				0,
				static_cast<uint32_t>(sizeof(Vertex3D)),
				VK_VERTEX_INPUT_RATE_VERTEX,
			};
			return binding_descr;
		}
		constexpr static std::array<const VkVertexInputAttributeDescription, 2> AttrDescr() noexcept {
			constexpr std::array< const VkVertexInputAttributeDescription, 2> result = {
				VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex3D, Position)) },
				VkVertexInputAttributeDescription{ 1, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex3D, UV)) },
			};
			return result;
		}

		bool operator==(const Vertex3D& other) const noexcept {
			return Position == other.Position && UV == other.UV;
		}
	};


	struct ColorVertex3D {
		glm::vec3 Position;
		glm::vec3 Color;
		glm::vec2 UV;

		constexpr static VkVertexInputBindingDescription BindingDescr() noexcept {
			const VkVertexInputBindingDescription binding_descr{
				0,
				static_cast<uint32_t>(sizeof(ColorVertex3D)),
				VK_VERTEX_INPUT_RATE_VERTEX,
			};
			return binding_descr;
		}

		constexpr static std::array<const VkVertexInputAttributeDescription, 3> AttrDescr() noexcept {
			constexpr std::array< const VkVertexInputAttributeDescription, 3> result = {
				VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(ColorVertex3D, Position)) },
				VkVertexInputAttributeDescription{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(ColorVertex3D, Color)) },
				VkVertexInputAttributeDescription{ 2, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<uint32_t>(offsetof(ColorVertex3D, UV)) },
			};
			return result;
		}
		
	};

	enum class VertexComponents {
		POS,
		NORM,
		COLOR,
		UV,
		TANGENT,
		BITANGENT,
		PADDING_FLOAT,
		PADDING_VEC4,
	};


	struct VertexLayout {
		VertexLayout(std::vector<VertexComponents>& components) : Components(components) {}
		VertexLayout() = default;
		std::vector<VertexComponents> Components;

		uint32_t Stride() const noexcept {
			uint32_t result = 0;
			for (const auto& cmp : Components) {
				switch (cmp) {
				case VertexComponents::UV:
					result += 2 * sizeof(float);
					break;
				case VertexComponents::PADDING_FLOAT:
					result += sizeof(float);
					break;
				case VertexComponents::PADDING_VEC4:
					result += 4 * sizeof(float);
					break;
				default:
					result += 3 * sizeof(float);
					break;
				}
			}
			return result;
		}


	};

}

#endif // !VULPES_VK_VERTEX_TYPES_H
