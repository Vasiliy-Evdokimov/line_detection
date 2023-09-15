/*
 * camera.hpp
 *
 *  Created on: Aug 17, 2023
 *      Author: vevdokimov
 */

#ifndef CAMERA_HPP_
#define CAMERA_HPP_

#include "opencv2/opencv.hpp"
#include <string.h>

#include "common_types.hpp"

using namespace std;

extern mutex parse_results_mtx[2];
extern ResultFixed parse_results[2];

struct ParseImageResult {
	int width;
	int height;
	bool fl_error;
	vector<cv::Point> res_points;
	vector<int> hor_ys;

	ParseImageResult() {
		width = 0;
		height = 0;
		fl_error = false;
		res_points.clear();
		hor_ys.clear();
	}

	ParseImageResult(const ParseImageResult& src) {
		width = src.width;
		height = src.height;
		fl_error = src.fl_error;
		res_points.clear();
		for (size_t i = 0; i < src.res_points.size(); i++)
			res_points.push_back(cv::Point(src.res_points[i]));
		for (size_t i = 0; i < src.hor_ys.size(); i++)
			hor_ys.push_back(src.hor_ys[i]);
	}

	ResultFixed ToFixed() {
		ResultFixed res;
		res.img_width = width;
		res.img_height = height;
		res.error_flag = fl_error ? 1 : 0;
		//
		res.max_points_count = MAX_POINTS_COUNT;
		res.points_count = res_points.size();
		memset(res.points, 0, sizeof(res.points));
		for (int i = 0; i < res.points_count; i++) {
			res.points[i].x = res_points[i].x;
			res.points[i].y = res_points[i].y;
		}
		//
		res.max_hor_count = MAX_HOR_COUNT;
		res.hor_count = hor_ys.size();
		memset(res.points_hor, 0, sizeof(res.points_hor));
		for (int i = 0; i < res.hor_count; i++)
			res.points_hor[i] = hor_ys[i];
		//
		return res;
	}
};

void parse_image(string aThreadName, cv::Mat imgColor,
	std::vector<cv::Point>& res_points, std::vector<int>& hor_ys,
	bool& fl_error, int aIndex);

void visualizer_func();

void camera_func(string aThreadName, string aCamAddress, int aIndex);

#endif /* CAMERA_HPP_ */
