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

using namespace std;

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
};

extern mutex parse_results_mtx;
extern ParseImageResult parse_results[2];

void parse_image(string aThreadName, cv::Mat imgColor,
	std::vector<cv::Point>& res_points, std::vector<int>& hor_ys,
	bool& fl_error, int aIndex);

void visualizer_func();

void camera_func(string aThreadName, string aCamAddress, int aIndex);

#endif /* CAMERA_HPP_ */
