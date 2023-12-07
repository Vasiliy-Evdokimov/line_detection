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

const int VK_KEY_A = 97;	//	влево
const int VK_KEY_B = 98;	//	сохранить точки пересечения
const int VK_KEY_C = 99;	//	сохранить точки пересечения в формате csv
const int VK_KEY_D = 100;	//	вправо
const int VK_KEY_F = 102;	//	выбор режима
const int VK_KEY_I = 105;	//	сформировать точки пересечения
const int VK_KEY_L = 108;	//	?
const int VK_KEY_M = 109;	//	?
const int VK_KEY_N = 110;	//	загрузить точки пересечения
const int VK_KEY_Q = 113;	//	?
const int VK_KEY_S = 115;	//	вниз
const int VK_KEY_W = 119;	//	вверх
const int VK_KEY_X = 120;	//	?

const int VK_KEY_UP = VK_KEY_W;
const int VK_KEY_DOWN = VK_KEY_S;
const int VK_KEY_LEFT = VK_KEY_A;
const int VK_KEY_RIGHT = VK_KEY_D;

const int VK_KEY_DEL = 255;

const int MODE_NOT_SELECTED = 0;
const int MODE_SELECT_LINE = 1;
const int MODE_SELECT_POINT = 2;
const int MODE_ADD_USER_LINE = 3;
const int MODE_ADD_USER_POINT = 4;
const int MODE_RULER = 5;

extern std::map<int, bool> keys_toggle;
extern std::map<int, string> modes_list;
extern std::set<int> current_modes;

struct CalibPoint {
	cv::Point2f point;
	cv::Point2f point_cnt;
	cv::Point2f point_mm;
	int col;
	int row;
	double angle_row; // угол между этой точкой и следующей в строке
	double angle_col; // угол между этой точкой и следующей в столбце
};

struct Line {
	Point2f pt1;
	Point2f pt2;
	Point2f	mid;
	int dir;
	int index;
	int type; // 1 - OpenCV; 2 - User;
};

struct CalibPointLine {
	std::vector<CalibPoint> points;
	int index;
};

extern cv::Mat cameraMatrix;
extern cv::Mat distCoeffs;

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args );

double getAngle(cv::Point pt1, cv::Point pt2);

Point2f getMiddle(cv::Point2f pt1, cv::Point2f pt2);

double getDistance(cv::Point2f pt1, cv::Point2f pt2);

Point2f getIntersection(cv::Point2f A, cv::Point2f B, cv::Point2f C, cv::Point2f D);

int get_vector_calib_point_index(std::vector<CalibPoint> aVector, CalibPoint aPoint);

void read_calibration();

void save_intersection_points();

void load_intersection_points();

void save_intersection_points_csv();

Point2f get_point_cnt(cv::Mat& img, Point2f aPoint);

void fill_sorted_cols_rows();

void fill_intersection_counted_fields(CalibPoint& aPoint);

void fill_opencv_intersections_lines(cv::Mat& img);

void fill_intersection_points(cv::Mat& img);

int get_point_quarter(Point2f pt);

int get_nearest_intersection_index(CalibPoint &pt);

CalibPointLine get_calib_point_line_by_index(std::vector<CalibPointLine> aVector, int aIndex);

void find_point_mm(CalibPoint &pt);

void onMouse(int event, int x, int y, int flags, void* userdata);

void handle_keys(cv::Mat& img);

void draw_intersection_points(cv::Mat& img);

void calibration(cv::Mat& img);

bool is_key_on(int aKey);

void toggle_key(int aKey);

int select_calib_line(int x, int y);

int select_calib_point(int x, int y);

void draw_ruler_points(cv::Mat& img);

int get_status_bar_height();

#endif /* CALIBRATION_HPP_ */
