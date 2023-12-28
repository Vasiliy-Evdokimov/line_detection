/*
 * common_types.hpp
 *
 *  Created on: 15 сент. 2023 г.
 *      Author: vevdokimov
 */

#ifndef COMMON_TYPES_HPP_
#define COMMON_TYPES_HPP_

#include <chrono>

#include "opencv2/core/types.hpp"

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

struct ParseImageResult
{
	int width;
	int height;
	//
	vector<cv::Point> res_points;
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

	ParseImageResult() {
		width = 0;
		height = 0;
		//
		res_points.clear();
		//
		center_x = 0;
		center_x_mm = 0;
		//
		hor_ys.clear();
		//
		fl_slow_zone = false;
		fl_stop_zone = false;
		fl_stop_mark = false;
		stop_mark_distance = 0;
		//
		fl_err_line = false;
		fl_err_parse = false;
		fl_err_camera = false;
	}

	ParseImageResult(const ParseImageResult& src) {
		width = src.width;
		height = src.height;
		//
		res_points.clear();
		for (size_t i = 0; i < src.res_points.size(); i++)
			res_points.push_back(cv::Point(src.res_points[i]));
		//
		center_x = src.center_x;
		center_x_mm = src.center_x_mm;
		//
		for (size_t i = 0; i < src.hor_ys.size(); i++)
			hor_ys.push_back(src.hor_ys[i]);
		//
		fl_slow_zone = src.fl_slow_zone;
		fl_stop_zone = src.fl_stop_zone;
		fl_stop_mark = src.fl_stop_mark;
		stop_mark_distance = src.stop_mark_distance;
		//
		fl_err_line = src.fl_err_line;
		fl_err_parse = src.fl_err_parse;
		fl_err_camera = src.fl_err_camera;
	}

	ResultFixed ToFixed() {
		ResultFixed res;
		res.img_width = width;
		res.img_height = height;
		//
		res.max_points_count = MAX_POINTS_COUNT;
		res.points_count = res_points.size();
		memset(res.points, 0, sizeof(res.points));
		for (int i = 0; i < res.points_count; i++) {
			res.points[i].x = res_points[i].x;
			res.points[i].y = res_points[i].y;
			if (i == (MAX_POINTS_COUNT - 1)) break;
		}
		//
		res.center_x = center_x;
		res.center_x_mm = center_x_mm;
		//
		res.max_hor_count = MAX_HOR_COUNT;
		res.hor_count = hor_ys.size();
		memset(res.points_hor, 0, sizeof(res.points_hor));
		for (int i = 0; i < res.hor_count; i++) {
			res.points_hor[i] = hor_ys[i];
			if (i == (MAX_HOR_COUNT - 1)) break;
		}
		//
		res.zone_flags = 0;
		res.zone_flags |= (fl_slow_zone << 2);
		res.zone_flags |= (fl_stop_zone << 1);
		res.zone_flags |= (fl_stop_mark << 0);
		//
		res.stop_distance = stop_mark_distance;
		//
		res.error_flags = 0;
		res.error_flags |= (fl_err_line << 0);
		res.error_flags |= (fl_err_parse << 1);
		res.error_flags |= (fl_err_camera << 2);
		//	3й бит - таймаут, выставляется при чтении из shared memory
		//
		return res;
	}
};


#endif /* COMMON_TYPES_HPP_ */
