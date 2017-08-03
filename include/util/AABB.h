#pragma once
#ifndef VULPES_UTIL_AABB_H
#define VULPES_UTIL_AABB_H

#include "vpr_stdafx.h"

namespace vulpes {

	namespace util {

		struct AABB {

			glm::vec3 Min, Max;
			
			glm::vec3 Extents() const;

			glm::vec3 Center() const;

			void UpdateMinMax(const float& y_min, const float& y_max);

			bool Intersection(const glm::vec3& origin, const glm::vec3& ray) const;

		};

		

	}

}

#endif // !VULPES_UTIL_AABB_H
