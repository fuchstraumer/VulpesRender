#include "vpr_stdafx.h"
#include "AABB.h"
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
	}

}

