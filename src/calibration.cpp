/*
 * calibration.cpp
 *
 *  Created on: 2 нояб. 2023 г.
 *      Author: vevdokimov
 */

#include "calibration.hpp"

using namespace std;

std::map<int, bool> keys_toggle = {
	{VK_KEY_A, false},
	{VK_KEY_S, false},
	{VK_KEY_X, false},
	{VK_KEY_L, false},
	{VK_KEY_UP, false},
	{VK_KEY_DOWN, false},
	{VK_KEY_LEFT, false},
	{VK_KEY_RIGHT, false},
	{VK_KEY_DEL, false}
};

std::vector<cv::Point2f> calib_pts;

int selected_pt_idx = -1;

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
        //
        if (is_key_on(VK_KEY_A)) {
        	selected_pt_idx = select_calib_pt(x, y);
        } else {
        	calib_pts.push_back(cv::Point(x, y));
        }
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

void handle_keys()
{
	if (!is_key_on(VK_KEY_A))
		selected_pt_idx = -1;
	//
	if (is_key_on(VK_KEY_L))
	{
		toggle_key(VK_KEY_L);
		write_log("points load");
		//
		load_calib_points();
	}
	//
	if (is_key_on(VK_KEY_S))
	{
		toggle_key(VK_KEY_S);
		write_log("points save");
		//
		save_calib_points();
	}
	//
	if (is_key_on(VK_KEY_X))
	{
		toggle_key(VK_KEY_X);
		write_log("points clear");
		//
		calib_pts.clear();
	}
	//
	if (is_key_on(VK_KEY_DEL))
	{
		toggle_key(VK_KEY_DEL);
		if (selected_pt_idx == -1) return;
		write_log("point " + to_string(selected_pt_idx) +  " delete");
		//
		calib_pts.erase(calib_pts.begin() + selected_pt_idx);
		selected_pt_idx = -1;
	}
	//
	if (is_key_on(VK_KEY_UP))
	{
		toggle_key(VK_KEY_UP);
		if (selected_pt_idx == -1) return;
		write_log("point " + to_string(selected_pt_idx) +  " up");
		//
		calib_pts[selected_pt_idx].y -= 1;
	}
	//
	if (is_key_on(VK_KEY_DOWN))
	{
		toggle_key(VK_KEY_DOWN);
		if (selected_pt_idx == -1) return;
		write_log("point " + to_string(selected_pt_idx) +  " down");
		//
		calib_pts[selected_pt_idx].y += 1;
	}
	//
	if (is_key_on(VK_KEY_LEFT))
	{
		toggle_key(VK_KEY_LEFT);
		if (selected_pt_idx == -1) return;
		write_log("point " + to_string(selected_pt_idx) +  " left");
		//
		calib_pts[selected_pt_idx].x -= 1;
	}
	//
	if (is_key_on(VK_KEY_RIGHT))
	{
		toggle_key(VK_KEY_RIGHT);
		if (selected_pt_idx == -1) return;
		write_log("point " + to_string(selected_pt_idx) +  " right");
		//
		calib_pts[selected_pt_idx].x += 1;
	}
}

void calib_points(cv::Mat& und)
{
	cv::Point cnt(und.cols / 2, und.rows / 2);
	//
	cv::line(und, cv::Point2f(cnt.x, 0), cv::Point2f(cnt.x, und.rows),
		CLR_YELLOW, 1, cv::LINE_AA, 0);
	cv::line(und, cv::Point2f(0, cnt.y), cv::Point2f(und.cols, cnt.y),
		CLR_YELLOW, 1, cv::LINE_AA, 0);
	//
	if (is_key_on(VK_KEY_A)) {
		putText(und, "SELECTION MODE", cv::Point(5, 20), 1, 1.5, CLR_MAGENTA, 2);
	}
	//
	draw_calib_pts(und);
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
	handle_keys();
}

bool is_key_on(int aKey)
{
	if (keys_toggle.count(aKey))
		return keys_toggle[aKey];
	return false;
}

void toggle_key(int aKey)
{
	//write_log("selected_pt_idx = " + to_string(selected_pt_idx));
	if (keys_toggle.count(aKey))
	{
		keys_toggle[aKey] = !keys_toggle[aKey];
		write_log(to_string(aKey) + " = " + (keys_toggle[aKey] ? "ON" : "OFF"));
	}
}

const int CALIB_PT_R = 6;

void draw_calib_pts(cv::Mat& img)
{
	cv::Scalar clr;
	for (size_t i = 0; i < calib_pts.size(); i++)
	{
		clr = (i != selected_pt_idx) ? CLR_RED : CLR_GREEN;
		//
		cv::circle(img, calib_pts[i], CALIB_PT_R, clr, 1, cv::LINE_AA);
		cv::line(img, calib_pts[i] - cv::Point2f(0, 10), calib_pts[i] + cv::Point2f(0, 10), clr, 1, cv::LINE_AA, 0);
		cv::line(img, calib_pts[i] - cv::Point2f(10, 0), calib_pts[i] + cv::Point2f(10, 0), clr, 1, cv::LINE_AA, 0);
	}
}

int select_calib_pt(int x, int y)
{
	for (size_t i = 0; i < calib_pts.size(); i++)
	{
		int x1 = calib_pts[i].x - CALIB_PT_R;
		int x2 = calib_pts[i].x + CALIB_PT_R;
		int y1 = calib_pts[i].y - CALIB_PT_R;
		int y2 = calib_pts[i].y + CALIB_PT_R;
		//
		if ((x >= x1) && ( x <= x2) && (y >= y1) && (y <= y2))
		{
			if (i != selected_pt_idx) return i;
				else return -1;
		}
	}
	return -1;
}




