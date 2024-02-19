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

#include "defines.hpp"
#include "common_types.hpp"

using namespace std;

extern ResultFixed parse_results[CAM_COUNT];

void visualizer_func();

void* p_visualizer_func(void *args);

void parse_image(string aThreadName, cv::Mat imgColor,
	ParseImageResult& parse_result, int aIndex,
	const bool slow_stop_found);

void camera_func(string aThreadName, string aCamAddress, int aIndex);

#endif /* CAMERA_HPP_ */
