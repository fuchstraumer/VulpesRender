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

			float tx1 = (Min.x - origin.x) * inv_ray.x;
			float tx2 = (Max.x - origin.x) * inv_ray.x;

			float tmin = std::min(tx1, tx2);
			float tmax = std::max(tx1, tx2);

			float ty1 = (Min.y - origin.y) * inv_ray.y;
			float ty2 = (Max.y - origin.y) * inv_ray.y;

			tmin = std::max(tmin, std::min(ty1, ty2));
			tmax = std::min(tmax, std::max(ty1, ty2));

			float tz1 = (Min.z - origin.z) * inv_ray.z;
			float tz2 = (Max.z - origin.z) * inv_ray.z;

			tmin = std::max(tmin, std::min(tz1, tz2));
			tmax = std::min(tmax, std::max(tz1, tz2));

			return tmax >= tmin;
			
		}
	}

}

