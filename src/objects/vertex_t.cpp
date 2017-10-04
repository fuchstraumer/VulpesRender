#include "vpr_stdafx.h"
#include "objects/vertex_t.hpp"

namespace vulpes {

    const VkVertexInputBindingDescription vertex_t::bindingDescription{
        0, (sizeof(glm::vec3) * 2) + sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX
    };

    const std::array<VkVertexInputAttributeDescription, 3> vertex_t::attributeDescriptions{
        VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
        VkVertexInputAttributeDescription{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3) },
        VkVertexInputAttributeDescription{ 2, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec3) * 2 }
    };

    vertex_t::vertex_t(const glm::vec3& _pos, const glm::vec3& _norm, const glm::vec2& _uv) : pos(_pos), normal(_norm), uv(_uv) {}

    vertex_t::vertex_t(const vertex_t& other) noexcept : pos(other.pos), normal(other.normal), uv(other.uv) {}

    vertex_t::vertex_t(vertex_t&& other) noexcept : pos(std::move(other.pos)), normal(std::move(other.normal)), uv(std::move(other.uv))  {}

    vertex_t& vertex_t::operator=(const vertex_t& other) noexcept {
        pos = other.pos;
        normal = other.normal;
        uv = other.uv;
        return *this;
    }

    vertex_t& vertex_t::operator=(vertex_t&& other) noexcept {
        pos = std::move(other.pos);
        normal = std::move(other.normal);
        uv = std::move(other.uv);
        return *this;
    }

    bool vertex_t::operator==(const vertex_t& other) const noexcept  {
        return (pos == other.pos) && (normal == other.normal) && (uv == other.uv);
    }


}