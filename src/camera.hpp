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

void parse_image(string aThreadName, cv::Mat imgColor,
	std::vector<cv::Point>& res_points, std::vector<int>& hor_ys,
	bool& fl_error, int aIndex);

void visualizer_func();

void camera_func(string aThreadName, string aCamAddress, int aIndex);

#endif /* CAMERA_HPP_ */
