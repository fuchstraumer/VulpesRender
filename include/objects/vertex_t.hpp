#pragma once
#ifndef VULPES_VERTEX_T_HPP
#define VULPES_VERTEX_T_HPP

#include "vpr_stdafx.h"
#include "glm/gtx/hash.hpp"

namespace vulpes {

    /** \ingroup Objects
    */
    struct vertex_t {
        vertex_t() = default;
        ~vertex_t() = default;

        vertex_t(const glm::vec3& _pos, const glm::vec3& _norm = glm::vec3(0.0f), const glm::vec2& _uv = glm::vec2(0.0f));
        vertex_t(const vertex_t& other) noexcept;
        vertex_t(vertex_t&& other) noexcept;

        vertex_t& operator=(const vertex_t&) noexcept;
        vertex_t& operator=(vertex_t&& other) noexcept;
        
        glm::vec3 pos = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0.0f);
        glm::vec2 uv = glm::vec2(0.0f);

        bool operator==(const vertex_t& other) const noexcept;
        
        static const VkVertexInputBindingDescription bindingDescription;

        static const std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;
    };
}

namespace std {
    /** Hash method ovverride so that we can use vulpes::vertex_t in standard library containers. Used with tinyobj to remove/avoid duplicated vertices. \ingroup Objects */
    template<>
    struct hash<vulpes::vertex_t> {
        size_t operator()(const vulpes::vertex_t& vert) const {
            return (hash<glm::vec3>()(vert.pos)) ^ (hash<glm::vec3>()(vert.normal) << 1) ^ (hash<glm::vec2>()(vert.uv) << 4);
        }
    };
}

#endif //!VULPES_VERTEX_T_HPP