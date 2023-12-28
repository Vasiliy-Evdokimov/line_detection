/*
 * horizontal.cpp
 *
 *  Created on: Aug 16, 2023
 *      Author: vevdokimov
 */

#include "opencv2/opencv.hpp"
#include "config.hpp"
#include "defines.hpp"
#include "contours.hpp"
#include "horizontal.hpp"

#define HORZ_DIST 20

bool check_points_horz(const cv::Point& pt1, const cv::Point& pt2)
{
	return abs(pt1.y - pt2.y) < HORZ_DIST;
}

bool check_rects_adj_vert(cv::Rect r1, const cv::Rect r2)
{
	// Если один прямоугольник находится ниже другого
	if ((r1.y > (r2.y + r2.height)) ||
		(r2.y > (r1.y + r1.height))) {
		return false;
	}
	// В противном случае, прямоугольники пересекаются
	return true;
}

bool check_rects_adj_horz(const cv::Rect r1, const cv::Rect r2)
{
	// Если один прямоугольник находится справа от другого
	if ((r1.x > (r2.x + r2.width)) ||
		(r2.x > (r1.x + r1.width))) {
		return false;
	}
	// В противном случае, прямоугольники пересекаются
	return true;
}

bool check_horz_bounds(std::vector<RectData*> line)
{
	for (size_t i = 0; i < line.size() - 1; i++)
	{
		bool fl = false;
		for (size_t j = i + 1; j < line.size(); j++) {
			if (check_rects_adj_horz(line[i]->bound, line[j]->bound)) {
				fl = true;
				break;
			}
		}
		if (!fl) return false;
	}
	return true;
}

int get_hor_line_y(cv::Mat& imgColor, std::vector<RectData*> line)
{
	int y = 0;
	for (size_t j = 0; j < line.size(); j++) {
		cv::Point pt = line[j]->center;
		y += pt.y;
		//
#ifndef NO_GUI
		if (config.DRAW && config.DRAW_DETAILED)
			cv::circle(imgColor, pt, 3, CLR_RED, 1, cv::LINE_AA);
#endif
	}
	y /= line.size();
	return y;
}

void find_horizontal(
	cv::Mat& imgColor,
	std::vector<RectData*>& buf_points,
	std::vector<int>& hor_ys
) {

	int imgOffsetV = imgColor.cols / config.NUM_ROI_V;

	std::vector<RectData*> hor_points;
	std::vector<std::vector<RectData*>> hor_lines;
	hor_ys.clear();

	// находим горизонтальные линии
	{
		size_t i = 0;
		std::vector<RectData*> line;
		RectData* rd1;
		RectData* rd2;
		while (i < buf_points.size()) {
			int k = 0;
			rd1 = buf_points[i];
			for (size_t j = i + 1; j < buf_points.size(); j++) {
				rd2 = buf_points[j];
				if (rd1->roi_row != rd2->roi_row) continue;
				//
				if (check_points_horz(rd1->center, rd2->center)) {
					if (line.size() == 0)
						line.push_back(rd1);
					line.push_back(rd2);
					k = j;
				}
			}
			if (k > 0)
				std::sort(line.begin(), line.end(),
					[](RectData* a, RectData* b) { return (a->bound.x < b->bound.x); });
			int last_idx = line.size() - 1;
			//
			bool line_flag =
				true
				//&& (line.size() >= NUM_ROI_V)	//	количество точек не меньше чем "столбцов"
				//&& check_horz_bounds(line)	//	области горизонтально смежные
				//
				&& (line.size() >= 2)
				&& (line[0]->roi_row == line[last_idx]->roi_row)					//	крайние точки находятся в одной строке
				&& ((line[last_idx]->roi_col - line[0]->roi_col) > 1)				//	крайние точки не находятся в смежных столбцах
				&& ((line[0]->bound.x + line[0]->bound.width) % imgOffsetV == 0)	//	самая левая область прижата к правому краю клетки
				&& ((line[last_idx]->bound.x) % imgOffsetV == 0)					//	самая праваяя область прижата к левому краю клетки
			;
			if (line_flag) {
				hor_lines.push_back(line);
				//
				for (size_t m = 0; m < line.size(); m++)
					hor_points.push_back(line[m]);
			}
			i = ((k > 0) && line_flag) ? (k + 1) : (i + 1);
			line.clear();
		}
	}

	//	усредняем соседние горизонтальные линии для переходов между рядами
	{
		size_t i = 0;
		while (i < hor_lines.size()) {
			int k = 0;
			int y1 = get_hor_line_y(imgColor, hor_lines[i]);
			int y_sum = y1;
			int y_cnt = 1;
			for (size_t j = i + 1; j < hor_lines.size(); j++) {
				int y2 = get_hor_line_y(imgColor, hor_lines[j]);
				//
				if (abs(y1 - y2) < config.HOR_COLLAPSE) {
					y_sum += y2;
					y_cnt++;
					k = j;
				}
			}
			hor_ys.push_back(y_sum / y_cnt);
			i = (k > 0) ? (k + 1) : (i + 1);
		}
	}

}
