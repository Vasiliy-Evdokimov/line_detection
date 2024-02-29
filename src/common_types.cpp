/*
 * common_types.cpp
 *
 *  Created on: 19 янв. 2024 г.
 *      Author: vevdokimov
 */

#include <string.h>

#include "common_types.hpp"

ParseImageResult::ParseImageResult()
{
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
	//
	pult_flags = 0;
	hidro_height = 0;
}

ParseImageResult::ParseImageResult(const ParseImageResult& src)
{
	width = src.width;
	height = src.height;
	//
	res_points.clear();
	for (size_t i = 0; i < src.res_points.size(); i++)
		res_points.push_back(ResultPoint(src.res_points[i]));
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
	//
	pult_flags = src.pult_flags;
	hidro_height = src.hidro_height;
}

ResultFixed ParseImageResult::ToFixed()
{
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
	res.pult_flags = pult_flags;
	res.hidro_height = hidro_height;
	//
	return res;
}
