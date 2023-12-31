/*
 * contours.hpp
 *
 *  Created on: Aug 16, 2023
 *      Author: vevdokimov
 */

#ifndef CONTOURS_HPP_
#define CONTOURS_HPP_

#include "opencv2/opencv.hpp"

struct RectData
{
	int dir;
	int _reserved;
	double len;
	cv::Point center;
	cv::Rect bound;
	int roi_row;
	int roi_col;
};

struct ContData
{
	cv::Rect roi;
	std::vector<RectData> vRect;
};

void get_contur_params(cv::Mat& img, cv::Rect& roi, ContData& data, int roi_row, int roi_col);
double length(const cv::Point& p1, const cv::Point& p2);
double angle(const cv::Point& p);
RectData* sort_cont(const cv::Point& base, ContData& data);

#endif /* CONTOURS_HPP_ */
