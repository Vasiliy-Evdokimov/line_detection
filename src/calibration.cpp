/*
 * calibration.cpp
 *
 *  Created on: 2 нояб. 2023 г.
 *      Author: vevdokimov
 */

#include "calibration.hpp"

std::vector<cv::Point2f> calib_pts;

int key_down = -1;
int dropped_pt_index = -1;

const string calib_points_file = "/home/vevdokimov/eclipse-workspace/line_detection/Debug/calib_points.xml";

const double D2R = ((2.0 * M_PI) / 360.0);
const double R2D = (180 / M_PI);

void pundistors(cv::Point2f &r, const cv::Point2f &a, double w, double h, double dist_fov) // , double dist, double fov
{
	float camWDelta = tan((w * 0.25) * D2R * D2R);
	float posXDelta = tan((a.x * 0.5) * D2R * D2R);
	float camXMid = 2.0 * (dist_fov * camWDelta);	//	(dist - fov)
	float ptX = 2.0 * (dist_fov * posXDelta);
	r.x = camXMid - ptX;
	//
	float camHDelta = tan((h * 0.25) * D2R * D2R);
	float posYDelta = tan((a.y * 0.5) * D2R * D2R);
	float camYMid = 2.0 * (dist_fov * camHDelta);
	float ptY = 2.0 * (dist_fov * posYDelta);
	r.y = camYMid - ptY;
}

void save_calib_points()
{
	cv::FileStorage file(calib_points_file, cv::FileStorage::WRITE);
	file << "calibPoints" << calib_pts;
	file.release();
}

void load_calib_points()
{
	cv::FileStorage file(calib_points_file, cv::FileStorage::READ);
	file["calibPoints"] >> calib_pts;
	file.release();
}

void onMouse(int event, int x, int y, int flags, void* userdata)
{
    if (event == cv::EVENT_LBUTTONDOWN)
    {
        std::cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << std::endl;
        //
        //if (calib_pts.size() == 2) calib_pts.clear();
        if (key_down == VK_KEY_A)
        {
        	write_log("with key 97");
        } else
        //
        	calib_pts.push_back(cv::Point(x, y));
    }
    else if (event == cv::EVENT_RBUTTONDOWN)
    {
        //	std::cout << "Right button of the mouse is clicked - position (" << x << ", " << y << ")" << std::endl;
    }
    else if (event == cv::EVENT_MBUTTONDOWN)
    {
    	//	std::cout << "Middle button of the mouse is clicked - position (" << x << ", " << y << ")" << std::endl;
    	calib_pts.erase(calib_pts.end() - 1);
    	//calib_pts.clear();
    }
    else if (event == cv::EVENT_MOUSEMOVE)
    {
    	//	std::cout << "Mouse move over the window - position (" << x << ", " << y << ")" << std::endl;
    }
}

void calib_points(cv::Mat& und, int key_down)
{

	cv::Point cnt(und.cols / 2, und.rows / 2);
	//
	cv::line(und, cv::Point2f(cnt.x, 0), cv::Point2f(cnt.x, und.rows),
		CLR_YELLOW, 1, cv::LINE_AA, 0);
	cv::line(und, cv::Point2f(0, cnt.y), cv::Point2f(und.cols, cnt.y),
		CLR_YELLOW, 1, cv::LINE_AA, 0);
	//
	for (size_t j = 0; j < calib_pts.size(); j++)
	{
		cv::circle(und, calib_pts[j], 6, CLR_RED, 1, cv::LINE_AA);
		cv::line(und, calib_pts[j] - cv::Point2f(0, 10), calib_pts[j] + cv::Point2f(0, 10), CLR_RED, 1, cv::LINE_AA, 0);
		cv::line(und, calib_pts[j] - cv::Point2f(10, 0), calib_pts[j] + cv::Point2f(10, 0), CLR_RED, 1, cv::LINE_AA, 0);
	}
	//
	if ((false) && (calib_pts.size() == 2)) {
		cv::line(und, calib_pts[0], calib_pts[1], CLR_RED, 1, cv::LINE_AA, 0);
		//
		double dist_fov = -1180;
		//
		cv::Point2f pt0, pt1;
		pundistors(pt0, calib_pts[0], und.cols, und.rows, dist_fov);
		pundistors(pt1, calib_pts[1], und.cols, und.rows, dist_fov);
		//
		double dist1 = GetPointDist(calib_pts[0], calib_pts[1]);
		double dist2 = GetPointDist(pt0, pt1);
		int y = 1, off = 15;
		//putText(und, to_string(dist1) + "px", calib_pts[0] + cv::Point2f(10, off * y++), 1, 1, CLR_RED);
		putText(und, to_string(dist2) + "mm", calib_pts[0] + cv::Point2f(10, off * y++), 1, 1, CLR_RED);
		//putText(und, to_string(calib_pts[0].y) + "y", calib_pts[0] + cv::Point2f(10, off * y++), 1, 1, CLR_RED);
		putText(und, to_string(24 / dist2) + "d", calib_pts[0] + cv::Point2f(10, off * y++), 1, 1, CLR_RED);
	}
	//
	if (key_down > 0) {
		if (key_down == VK_KEY_S)
		{
			write_log("save");
			//save_calib_points();
		} else
		//
		if (key_down == VK_KEY_X)
		{
			write_log("points clear");
			//calib_pts.clear();
		} else
		//
		if (key_down == VK_KEY_A)
		{
			write_log("a pressed");
		} else
		//
		write_log(to_string(key_down));
	}

}



