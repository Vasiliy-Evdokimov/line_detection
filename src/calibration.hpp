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
const int VK_KEY_B = 98;
const int VK_KEY_C = 99;
const int VK_KEY_I = 105;
const int VK_KEY_L = 108;
const int VK_KEY_N = 110;
const int VK_KEY_Q = 113;
const int VK_KEY_S = 115;
const int VK_KEY_X = 120;

const int VK_KEY_UP = 114; 		//r //82;
const int VK_KEY_DOWN = 102;	//f //84;
const int VK_KEY_LEFT = 100;	//d //81;
const int VK_KEY_RIGHT = 103;	//g //83;

const int VK_KEY_DEL = 255;

extern std::map<int, bool> keys_toggle;

struct calib_point {
	cv::Point2f point;
	cv::Point2f point_cnt;
	cv::Point2f point_mm;
	int col;
	int row;
	int quarter;
	double angle_row; // угол между этой точкой и следующей в строке
	double angle_col; // угол между этой точкой и следующей в столбце
};

struct Line {
	Point2f pt1;
	Point2f pt2;
	Point2f	mid;
	int dir;
	int index;
};

struct CalibPointLine {
	std::vector<calib_point> points;
	int index;
};

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args );

void pundistors(cv::Point2f &r, const cv::Point2f &a, double w, double h, double dist_fov);

double getAngle(cv::Point pt1, cv::Point pt2);

Point2f getMiddle(cv::Point2f pt1, cv::Point2f pt2);

double getDistance(cv::Point2f pt1, cv::Point2f pt2);

Point2f getIntersection(cv::Point2f A, cv::Point2f B, cv::Point2f C, cv::Point2f D);

int get_vector_calib_point_index(std::vector<calib_point> aVector, calib_point aPoint);

void save_manual_calib_points();

void load_manual_calib_points();

void save_intersection_points();

void load_intersection_points();

void save_intersection_points_csv();

Point2f get_point_cnt(cv::Mat& img, Point2f aPoint);

void fill_sorted_cols_rows();

void fill_intersection_counted_fields(calib_point& aPoint);

void fill_intersection_points(cv::Mat& img);

int get_point_quarter(Point2f pt);

int get_nearest_intersection_index(calib_point &pt);

CalibPointLine get_calib_point_line_by_index(std::vector<CalibPointLine> aVector, int aIndex);

void find_point_mm(calib_point &pt);

void onMouse(int event, int x, int y, int flags, void* userdata);

void handle_keys(cv::Mat& img);

void draw_intersection_points(cv::Mat& img);

void calib_points(cv::Mat& img);

bool is_key_on(int aKey);

void toggle_key(int aKey);

void draw_manual_calib_points(cv::Mat& img);

int select_calib_pt(int x, int y);

void draw_rule_points(cv::Mat& img);



#endif /* CALIBRATION_HPP_ */
