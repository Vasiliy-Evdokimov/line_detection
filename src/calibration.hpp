/*
 * calibration.hpp
 *
 *  Created on: 2 нояб. 2023 г.
 *      Author: vevdokimov
 */

#ifndef CALIBRATION_HPP_
#define CALIBRATION_HPP_

#include "log.hpp"

#include "opencv2/opencv.hpp"

#include <string.h>

#include "barcode.hpp"

using namespace cv;
using namespace std;

const int VK_KEY_A = 97;
const int VK_KEY_S = 115;
const int VK_KEY_X = 220;

extern std::vector<cv::Point2f> calib_pts;

void save_calib_points();

void load_calib_points();

void onMouse(int event, int x, int y, int flags, void* userdata);

void calib_points(cv::Mat& und, int key_down);

#endif /* CALIBRATION_HPP_ */
