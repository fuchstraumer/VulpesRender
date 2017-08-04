#include "vpr_stdafx.h"
#include "util/AABB.h"
namespace vulpes {

	namespace util {

		glm::vec3 vulpes::util::AABB::Extents() const {
			return (Min - Max);
		}

		glm::vec3 vulpes::util::AABB::Center() const {
			return (Min + Max) / 2.0f;
		}
		
		void AABB::UpdateMinMax(const float & y_min, const float & y_max) {
			Min.y = y_min;
			Max.y = y_max;
		}

		bool AABB::Intersection(const glm::vec3& origin, const glm::vec3& ray) const {
			
			glm::vec3 inv_ray = glm::vec3(1.0f) / ray;
			
			float txmin, txmax, tymin, tymax, tzmin, tzmax;
			txmin = (Min.x - origin.x) * inv_ray.x;
			txmax = (Max.x - origin.x) * inv_ray.x;
			tymin = (Min.y - origin.y) * inv_ray.y;
			tymax = (Max.y - origin.y) * inv_ray.y;
			tzmin = (Min.z - origin.z) * inv_ray.z;
			tzmax = (Max.z - origin.z) * inv_ray.z;

			float tmin = std::max(std::max(std::min(txmin, txmax), std::min(tymin, tymax)), std::min(tzmin, tzmax));
			float tmax = std::min(std::min(std::max(txmin, txmax), std::max(tymin, tymax)), std::max(tzmin, tzmax));
			
			if (tmax < 0.0f) {
				return false;
			}

			if (tmin > tmax) {
				return false;
			}

			return true;

		}
	}

}

