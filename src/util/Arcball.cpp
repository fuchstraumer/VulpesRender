#include "vpr_stdafx.h"
#include "util/Arcball.h"

namespace vulpes {

	Arcball::Arcball(const size_t & window_width, const size_t & window_height) : windowWidth(window_width), windowHeight(window_height), angle(0.0f), cameraAxis(0.0f, 1.0f, 0.0f), rollSpeed(0.2f) {}

}
