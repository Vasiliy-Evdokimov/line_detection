/*
 * common_types.hpp
 *
 *  Created on: 15 сент. 2023 г.
 *      Author: vevdokimov
 */

#ifndef COMMON_TYPES_HPP_
#define COMMON_TYPES_HPP_

#include <chrono>
#include <vector>

#include "defines.hpp"

using namespace std;
using namespace chrono;

typedef unsigned char uchar;

struct ResultPoint {
	int16_t x;
	int16_t y;
};

const int RESULT_TIME_SIZE = sizeof(high_resolution_clock::time_point);

struct DebugContourInfo
{
	int16_t type;
	ResultPoint center;
	ResultPoint left_top;
	int16_t width;
	int16_t height;
	int16_t length;
};

struct DebugFixed
{
	int16_t contours_size;
	DebugContourInfo contours[DEBUG_MAX_CONTOURS];
	int16_t image_size;
	uchar image[DEBUG_MAX_IMG_SIZE];
};

struct ResultFixed
{
	int16_t img_width;
	int16_t img_height;
	int16_t error_flags;
	//
	int16_t max_points_count;
	int16_t points_count;
	ResultPoint points[MAX_POINTS_COUNT];
	//
	int16_t max_hor_count;
	int16_t hor_count;
	int16_t points_hor[MAX_HOR_COUNT];
	//
	int16_t zone_flags;
	int16_t stop_distance;
	//
	int16_t center_x;
	int16_t center_x_mm;
	//
	int8_t result_time_point[RESULT_TIME_SIZE];
	//
	int16_t pult_flags;
	int16_t hidro_height;
};

struct ParseImageResult
{
	int width;
	int height;
	//
	vector<ResultPoint> res_points;
	//
	int16_t center_x;
	int16_t center_x_mm;
	//
	vector<int> hor_ys;
	//
	bool fl_slow_zone;
	bool fl_stop_zone;
	bool fl_stop_mark;
	int stop_mark_distance;
	//
	bool fl_err_line;
	bool fl_err_parse;
	bool fl_err_camera;
	//
	int16_t pult_flags;
	int16_t hidro_height;

	ParseImageResult();
	ParseImageResult(const ParseImageResult& src);
	ResultFixed ToFixed();
};


#endif /* COMMON_TYPES_HPP_ */
