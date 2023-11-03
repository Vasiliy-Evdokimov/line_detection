/*
 * calibration.hpp
 *
 *  Created on: 2 нояб. 2023 г.
 *      Author: vevdokimov
 */

#ifndef CALIBRATION_HPP_
#define CALIBRATION_HPP_

#include "defines.hpp"
#include "log.hpp"

#include "opencv2/opencv.hpp"

#include <string.h>

#include "barcode.hpp"

using namespace cv;
using namespace std;

const int VK_KEY_A = 97;
const int VK_KEY_S = 115;
const int VK_KEY_X = 120;
const int VK_KEY_L = 108;

const int VK_KEY_UP = 114; 		//r //82;
const int VK_KEY_DOWN = 102;	//f //84;
const int VK_KEY_LEFT = 100;	//d //81;
const int VK_KEY_RIGHT = 103;	//g //83;

const int VK_KEY_DEL = 255;

extern std::map<int, bool> keys_toggle;

extern std::vector<cv::Point2f> calib_pts;

bool is_key_on(int aKey);

void toggle_key(int aKey);

void draw_calib_pts(cv::Mat& img);

int select_calib_pt(int x, int y);

void save_calib_points();

void load_calib_points();

void onMouse(int event, int x, int y, int flags, void* userdata);

void calib_points(cv::Mat& und);

#endif /* CALIBRATION_HPP_ */
