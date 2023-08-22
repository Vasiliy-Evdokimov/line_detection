/*
 * contours.cpp
 *
 *  Created on: Aug 16, 2023
 *      Author: vevdokimov
 */

#include "defines.hpp"
#include "contours.hpp"

void get_contur_params(cv::Mat& img, cv::Rect& roi, ContData& data, double min_cont_len, int roi_row, int roi_col)
{
	cv::Mat roiImg;
	std::vector<std::vector<cv::Point2i>> cont;
	std::vector<cv::Vec4i> hie;

	img(roi).copyTo(roiImg);

	cv::findContours(roiImg, cont, hie, cv::RETR_CCOMP, cv::CHAIN_APPROX_NONE);

	for (auto i = cont.begin(); i != cont.end(); ++i)
	{
		RectData rd;

		cv::Moments M = cv::moments(*i);

		if (M.m00 < min_cont_len)
			continue;

		rd.len = M.m00;
		rd.center.x = int(M.m10 / M.m00) + roi.x;
		rd.center.y = int(M.m01 / M.m00) + roi.y;
		rd.bound = cv::boundingRect(*i);
		rd.bound.x += roi.x;
		rd.bound.y += roi.y;

		rd.roi_row = roi_row;
		rd.roi_col = roi_col;

		data.vRect.push_back(rd);
	}
}

double length(const cv::Point& p1, const cv::Point& p2)
{
	double x = static_cast<double>(p2.x - p1.x);
	double y = static_cast<double>(p2.y - p1.y);
	return std::sqrt(x * x + y * y);
}

double angle(const cv::Point& p)
{
	double x = static_cast<double>(p.x);
	double y = static_cast<double>(p.y);

	if ((x == 0.0) && (y == 0.0)) return 0.0;
	if (x == 0.0) return ((y > 0.0) ? 90 : 270);
	double theta = std::atan(y / x);
	theta *= 360.0 / (2.0 * M_PI);
	if (x > 0.0) return ((y >= 0.0) ? theta : 360.0 + theta);
	return (180 + theta);
}

RectData* sort_cont(const cv::Point& base, ContData& data)
{
	RectData* center(nullptr);
	double min = 100000.0;

	for (auto i = data.vRect.begin(); i != data.vRect.end(); ++i)
	{
		double len = length(base, i->center);
		if (len < min)
		{
			center = &(*i);
			min = len;
		}
	}

	return center;
}

void get_contour(cv::Mat& imgColor, cv::Mat& imgGray, cv::Rect& roi, ContData& dataItem, int roi_row, int roi_col)
{

#ifndef NO_GUI
	if (DRAW && DRAW_GRID)
		cv::rectangle(imgColor, roi, CLR_YELLOW, 1, cv::LINE_AA, 0);
#endif

	get_contur_params(imgGray, roi, dataItem, MIN_CONT_LEN, roi_row, roi_col);

}

