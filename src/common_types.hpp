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

struct ResultPoint {
	int16_t x;
	int16_t y;
};

const int RESULT_TIME_SIZE = sizeof(high_resolution_clock::time_point);

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
};

struct MyPoint
{
	int x;
	int y;
};

struct ParseImageResult
{
	int width;
	int height;
	//
	vector<MyPoint> res_points;
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

	ParseImageResult();
	ParseImageResult(const ParseImageResult& src);
	ResultFixed ToFixed();
};


#endif /* COMMON_TYPES_HPP_ */
