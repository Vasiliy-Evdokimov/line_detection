/*
 * common_types.hpp
 *
 *  Created on: 15 сент. 2023 г.
 *      Author: vevdokimov
 */

#ifndef COMMON_TYPES_HPP_
#define COMMON_TYPES_HPP_

#include "defines.hpp"

using namespace std;

struct ResultPoint {
	uint16_t x;
	uint16_t y;
};

struct ResultFixed {
	uint16_t img_width;
	uint16_t img_height;
	uint16_t error_flag;
	//
	uint8_t max_points_count;
	uint8_t points_count;
	ResultPoint points[MAX_POINTS_COUNT];
	//
	uint8_t max_hor_count;
	uint8_t hor_count;
	uint16_t points_hor[MAX_HOR_COUNT];
};

#endif /* COMMON_TYPES_HPP_ */
