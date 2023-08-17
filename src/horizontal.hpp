/*
 * horizontal.hpp
 *
 *  Created on: Aug 16, 2023
 *      Author: vevdokimov
 */

#ifndef HORIZONTAL_HPP_
#define HORIZONTAL_HPP_

#include "opencv2/opencv.hpp"
#include "contours.hpp"

bool check_points_horz(const cv::Point& pt1, const cv::Point& pt2, const double angle);
bool check_rects_adj_vert(cv::Rect r1, const cv::Rect r2);
bool check_rects_adj_horz(const cv::Rect r1, const cv::Rect r2);
bool check_horz_bounds(std::vector<RectData*> line);
int get_hor_line_y(cv::Mat& imgColor, std::vector<RectData*> line);
void find_horizontal(cv::Mat& imgColor,	std::vector<RectData*>& buf_points,	std::vector<int>& hor_ys);

#endif /* HORIZONTAL_HPP_ */
