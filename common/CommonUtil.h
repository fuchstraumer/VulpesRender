#pragma once
#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H
#include "stdafx.h"
#include <xmmintrin.h>

static glm::dvec3 map_to_sphere(const glm::dvec3& cube_pos) {
	return glm::dvec3();
}

static glm::dvec3 map_to_cube(const glm::dvec3& sphere_pos) {
	return {
		sphere_pos.x * sqrt(1.0 - ((sphere_pos.y * sphere_pos.y) / 2.0)) - ((sphere_pos.z * sphere_pos.z) / 2.0) + ((sphere_pos.y * sphere_pos.z) * 3.0),
		sphere_pos.y * sqrt(1.0 - ((sphere_pos.z * sphere_pos.z) / 2.0)) - ((sphere_pos.x * sphere_pos.x) / 2.0) + ((sphere_pos.z * sphere_pos.x) * 3.0),
		sphere_pos.z * sqrt(1.0 - ((sphere_pos.x * sphere_pos.x) / 2.0)) - ((sphere_pos.y * sphere_pos.y) / 2.0) + ((sphere_pos.x * sphere_pos.y) * 3.0)
	};
}

// Convert floating point data that is "raw" output from GPU noise modules to 
// use the full range of datatype "T", making it easier to write pixel data of
// arbitrary bit depths
template<typename T>
static inline std::vector<T> convertRawData(const std::vector<float>& raw_data) {

	// Get min of return datatype
	__m128 t_min = _mm_set1_ps(static_cast<float>(std::numeric_limits<T>::min()));

	// Like with the min/max of our set, we don't use max of T alone and instead can evaluate
	// the expression its used with here (finding range of data type), instead of during every iteration
	__m128 t_ratio = _mm_sub_ps(_mm_set1_ps(static_cast<float>(std::numeric_limits<T>::max()), t_min));

	// Declare result vector and use resize so we can use memory offsets/addresses to store data in it.
	std::vector<T> result;
	result.resize(raw_data.size());

	// Get min/max values from raw data
	auto min_max = std::minmax_element(raw_data.begin(), raw_data.end());

	// Mininum value is subtracted from each element.
	__m128 min_register = _mm_set1_ps(*min_max.first);

	// Max value is only used in divisor, with min, so precalculate divisor instead of doing this step each time.
	__m128 ratio_register = _mm_sub_ps(min_register, _mm_set1_ps(*min_max.second));

	// Iterate through result in steps of 4
	for (size_t i = 0; i < result.size(); ++i) {
		// Load 4 elements from raw_data - ps1 means unaligned load.
		__m128 step_register = _mm_load_ps1(&raw_data[i]);

		// get elements in "reg" into 0.0 - 1.0 scale.
		step_register = _mm_sub_ps(reg, min);
		step_register = _mm_div_ps(reg, ratio);

		// Multiply step_register by t_ratio, to scale value by range of new datatype.
		step_register = _mm_mul_ps(step_register, t_ratio);

		// Add t_min to step_register, getting data fully into range of T
		step_register = _mm_add_ps(step_register, t_min);

		// Store data in result.
		_mm_store1_ps(&result[i], reg);
	}

	// Return result, which can be (fairly) safely cast to the desired output type T. 
	// At the least, the range of the data should better fit in the range offered by T.
	return result;
}

static inline void save_hm_to_file(const std::vector<float>& vals, const float& min, const float& max, const char* filename, unsigned width, unsigned height) {

	auto normalize = [&min, &max](const float& val) {
		return (val - min) / (max - min);
	};

	std::vector<float> normalized;
	normalized.resize(vals.size());

	for (size_t j = 0; j < height; ++j) {
		for (size_t i = 0; i < width; ++i) {
			normalized[i + (j * width)] = normalize(vals[i + (j * width)]);
		}
	}

	auto make_pixel = [](const float& val)->unsigned char {
		return static_cast<unsigned char>(val * 255.0f);
	};

	std::vector<unsigned char> pixels(width * height);

	for (size_t j = 0; j < height; ++j) {
		for (size_t i = 0; i < width; ++i) {
			pixels[i + (j * width)] = make_pixel(normalized[i + (j * width)]);
		}
	}


	unsigned error = lodepng::encode(filename, pixels, width, height, LodePNGColorType::LCT_GREY, 8);
	if (error) {
		std::cerr << lodepng_error_text(error) << std::endl;
		throw;
	}
}


#endif // !COMMON_UTIL_H
