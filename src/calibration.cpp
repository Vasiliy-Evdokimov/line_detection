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

std::vector<calib_point> calib_pts;

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

void recount_cols_rows(cv::Point2f center)
{
	const int MAX_OFFSET = 20;

	std::sort(calib_pts.begin(), calib_pts.end(),
		[] (const calib_point& a, const calib_point& b) { return a.point.x < b.point.x; });

	for (size_t i = 1; i < calib_pts.size(); i++)
		calib_pts[i].col =
			(abs(calib_pts[i].point.x - calib_pts[i - 1].point.x) > MAX_OFFSET)
			? calib_pts[i - 1].col + 1
			: calib_pts[i - 1].col;

	std::sort(calib_pts.begin(), calib_pts.end(),
		[] (const calib_point& a, const calib_point& b) { return a.point.y > b.point.y; });

	for (size_t i = 1; i < calib_pts.size(); i++)
		calib_pts[i].row =
			(abs(calib_pts[i].point.y - calib_pts[i - 1].point.y) > MAX_OFFSET)
			? calib_pts[i - 1].row + 1
			: calib_pts[i - 1].row;

}


void save_calib_points()
{
	std::vector<cv::Point2f> calib_pts_for_file;

	for (size_t i = 0; i < calib_pts.size(); i++)
		calib_pts_for_file.push_back(calib_pts[i].point);

	cv::FileStorage file(calib_points_file, cv::FileStorage::WRITE);
	file << "calibPoints" << calib_pts_for_file;
	file.release();
}


void load_calib_points()
{
	std::vector<cv::Point2f> calib_pts_for_file;
	cv::FileStorage file(calib_points_file, cv::FileStorage::READ);
	file["calibPoints"] >> calib_pts_for_file;
	file.release();
	//
	calib_pts.clear();
	for (size_t i = 0; i < calib_pts_for_file.size(); i++)
		calib_pts.push_back({calib_pts_for_file[i], 0, 0});
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
        	calib_pts.push_back({ cv::Point(x, y), 0, 0 });
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

void handle_keys(cv::Mat& img)
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
		recount_cols_rows(Point2f(img.cols / 2, img.rows / 2));
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
		calib_pts[selected_pt_idx].point.y -= 1;
	}
	//
	if (is_key_on(VK_KEY_DOWN))
	{
		toggle_key(VK_KEY_DOWN);
		if (selected_pt_idx == -1) return;
		write_log("point " + to_string(selected_pt_idx) +  " down");
		//
		calib_pts[selected_pt_idx].point.y += 1;
	}
	//
	if (is_key_on(VK_KEY_LEFT))
	{
		toggle_key(VK_KEY_LEFT);
		if (selected_pt_idx == -1) return;
		write_log("point " + to_string(selected_pt_idx) +  " left");
		//
		calib_pts[selected_pt_idx].point.x -= 1;
	}
	//
	if (is_key_on(VK_KEY_RIGHT))
	{
		toggle_key(VK_KEY_RIGHT);
		if (selected_pt_idx == -1) return;
		write_log("point " + to_string(selected_pt_idx) +  " right");
		//
		calib_pts[selected_pt_idx].point.x += 1;
	}
}

double findAngle(cv::Point pt1, cv::Point pt2) {
    double angle = atan2(pt2.y - pt1.y, pt2.x - pt1.x) * 180 / CV_PI;
    return angle;
}

cv::Point2f middle(cv::Point2f pt1, cv::Point2f pt2) {
    return cv::Point(
        double(pt1.x + pt2.x) / 2,
        double(pt1.y + pt2.y) / 2
    );
}

double distance(cv::Point2f pt1, cv::Point2f pt2) {
    double dx = pt2.x - pt1.x;
    double dy = pt2.y - pt1.y;
    return std::sqrt(dx * dx + dy * dy);
}

cv::Point2f findIntersection(cv::Point2f A, cv::Point2f B, cv::Point2f C, cv::Point2f D) {
    cv::Point2f intersection(-100, -100);
    //
    float d = (A.x - B.x) * (C.y - D.y) - (A.y - B.y) * (C.x - D.x);
    //
    if (d != 0) {
        intersection.x = ((A.x * B.y - A.y * B.x) * (C.x - D.x) - (A.x - B.x) * (C.x * D.y - C.y * D.x)) / d;
        intersection.y = ((A.x * B.y - A.y * B.x) * (C.y - D.y) - (A.y - B.y) * (C.x * D.y - C.y * D.x)) / d;
    }
    //
    return intersection;
}

struct Intersections;

struct Line {
	Point2f pt1;
	Point2f pt2;
	Point2f	mid;
	int dir;
	std::vector<Intersections> IntersectionsArray;
	int index;
};

struct Intersections {
	Line line;
	Point2f points;
};

void calib_points(cv::Mat& img)
{

	const int CANNY_LOW = 50; //50; //250;      //  нижняя граница распознавания контуров
	const int CANNY_HIGH = 150; //150; //350;     //  верхняя     -//-
	const int HOUGH_LEVEL = 100; // 150 //30;     //  уровень расопзнавания прямых линий на контурах

	Mat grayscale;
	cvtColor(img, grayscale, COLOR_BGR2GRAY);
	//
	Mat binary;
	threshold(grayscale, binary, 128, 255, THRESH_BINARY);
	//
	cv::Mat edges;
	cv::Canny(binary, edges, CANNY_LOW, CANNY_HIGH);
	//
	//cv::imshow("Canny Edges", edges);
	//
	std::vector<cv::Vec2f> lines;
	cv::HoughLines(edges, lines, 1, CV_PI / 180, HOUGH_LEVEL, 0, 0);

	cv::Point cnt(img.cols / 2, img.rows / 2);

	std::vector<double> angles;
	std::vector<Line> lines_arr;

	Line cnt_h{Point2f(0, cnt.y), Point2f(img.cols, cnt.y), cnt, 1};
	lines_arr.push_back(cnt_h);
	Line cnt_v{Point2f(cnt.x, 0), Point2f(cnt.x, img.rows), cnt, 2};
	lines_arr.push_back(cnt_v);

	for (size_t i = 0; i < lines.size(); i++)
	{

		float rho = lines[i][0];
		float theta = lines[i][1];

		double a = cos(theta);
		double b = sin(theta);
		double x0 = a * rho;
		double y0 = b * rho;

		Point2f pt1(cvRound(x0 + 1000 * (-b)), cvRound(y0 + 1000 * (a)));
		Point2f pt2(cvRound(x0 - 1000 * (-b)), cvRound(y0 - 1000 * (a)));

		double angle = abs(findAngle(pt1, pt2));
		if ((angle != 0) && (angle < 50)) continue;

		Line new_line{ pt1, pt2, middle(pt1, pt2), ((angle == 0) ? 1 : 2) };

		bool fl = true;
		for (int j = 0; j < lines_arr.size(); j++)
			if (new_line.dir == lines_arr[j].dir) {
				if (((new_line.dir == 1) and (abs(new_line.mid.y - lines_arr[j].mid.y) < 10)) ||
				    ((new_line.dir == 2) and (abs(new_line.mid.x - lines_arr[j].mid.x) < 10)))
				{
					fl = false;
					break;
				}
			}

		if (fl) lines_arr.push_back(new_line);

	}

	int index = 0;
	int cnt_h_indx, cnt_v_indx;
	std::sort(lines_arr.begin(), lines_arr.end(),
		[] (const Line& a, const Line& b) { return a.mid.x < b.mid.x; });
	for (size_t i = 0; i < lines_arr.size(); i++)
		if (lines_arr[i].dir == 2) {
			if (lines_arr[i].mid == cnt_v.mid)
				cnt_v_indx = index;
			lines_arr[i].index = index++;
		}
	//
	index = 0;
	std::sort(lines_arr.begin(), lines_arr.end(),
			[] (const Line& a, const Line& b) { return a.mid.y < b.mid.y; });
	for (size_t i = 0; i < lines_arr.size(); i++)
		if (lines_arr[i].dir == 1) {
			if (lines_arr[i].mid == cnt_h.mid)
				cnt_h_indx = index;
			lines_arr[i].index = index++;
		}

	for (size_t i = 0; i < lines_arr.size(); i++)
		if (lines_arr[i].dir == 1)
			lines_arr[i].index = -1 * (lines_arr[i].index - cnt_h_indx);
		else
			lines_arr[i].index -= cnt_v_indx;

	std::vector<calib_point> intersections;
	for (size_t i = 0; i < lines_arr.size(); i++)
	{
		Line line1 = lines_arr[i];
		line(img, line1.pt1, line1.pt2, CLR_RED, 1, LINE_AA);
		//
		for (size_t j = i + 1; j < lines_arr.size(); j++)
		{
			Line line2 = lines_arr[j];
			if (line1.dir == line2.dir) continue;
			cv::Point2f intersection = findIntersection(line1.pt1, line1.pt2, line2.pt1, line2.pt2);
			if (intersection.x < 0) continue;
			calib_point cp;
			cp.point = intersection;
			cp.col = (line1.dir == 2) ? line1.index : line2.index;
			cp.row = (line1.dir == 1) ? line1.index : line2.index;
			intersections.push_back(cp);
		}
	}

	for (size_t i = 0; i < intersections.size(); i++)
	{
		calib_point cp = intersections[i];
		cv::circle(img, cp.point, 6, CLR_RED, 1, cv::LINE_AA);
		putText(img, to_string(cp.col) + ";" + to_string(cp.row),
			cp.point + cv::Point2f(3, ((cp.col % 2 == 0) ? 8 : 16)), 1, 0.6, CLR_GREEN);
	}

	cv::line(img, cv::Point2f(cnt.x, 0), cv::Point2f(cnt.x, img.rows),
		CLR_YELLOW, 1, cv::LINE_AA, 0);
	cv::line(img, cv::Point2f(0, cnt.y), cv::Point2f(img.cols, cnt.y),
		CLR_YELLOW, 1, cv::LINE_AA, 0);

	if (is_key_on(VK_KEY_A)) {
		putText(img, "SELECTION MODE", cv::Point(5, 20), 1, 1.5, CLR_MAGENTA, 2);
	}
	//
	//draw_calib_pts(img);
	//
	if ((false) && (calib_pts.size() == 2)) {
		cv::line(img, calib_pts[0].point, calib_pts[1].point, CLR_RED, 1, cv::LINE_AA, 0);
		//
		double dist_fov = -1180;
		//
		cv::Point2f pt0, pt1;
		pundistors(pt0, calib_pts[0].point, img.cols, img.rows, dist_fov);
		pundistors(pt1, calib_pts[1].point, img.cols, img.rows, dist_fov);
		//
		double dist1 = GetPointDist(calib_pts[0].point, calib_pts[1].point);
		double dist2 = GetPointDist(pt0, pt1);
		int y = 1, off = 15;
		//putText(img, to_string(dist1) + "px", calib_pts[0] + cv::Point2f(10, off * y++), 1, 1, CLR_RED);
		putText(img, to_string(dist2) + "mm", calib_pts[0].point + cv::Point2f(10, off * y++), 1, 1, CLR_RED);
		//putText(img, to_string(calib_pts[0].y) + "y", calib_pts[0] + cv::Point2f(10, off * y++), 1, 1, CLR_RED);
		putText(img, to_string(24 / dist2) + "d", calib_pts[0].point + cv::Point2f(10, off * y++), 1, 1, CLR_RED);
	}
	//
	handle_keys(img);
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
		cv::circle(img, calib_pts[i].point, CALIB_PT_R, clr, 1, cv::LINE_AA);
		cv::line(img, calib_pts[i].point - cv::Point2f(0, 10), calib_pts[i].point + cv::Point2f(0, 10), clr, 1, cv::LINE_AA, 0);
		cv::line(img, calib_pts[i].point - cv::Point2f(10, 0), calib_pts[i].point + cv::Point2f(10, 0), clr, 1, cv::LINE_AA, 0);
		putText(img, to_string(calib_pts[i].col) + ";" + to_string(calib_pts[i].row),
			calib_pts[i].point + cv::Point2f(10, 10), 1, 1, CLR_RED);
	}
}

int select_calib_pt(int x, int y)
{
	for (size_t i = 0; i < calib_pts.size(); i++)
	{
		int x1 = calib_pts[i].point.x - CALIB_PT_R;
		int x2 = calib_pts[i].point.x + CALIB_PT_R;
		int y1 = calib_pts[i].point.y - CALIB_PT_R;
		int y2 = calib_pts[i].point.y + CALIB_PT_R;
		//
		if ((x >= x1) && ( x <= x2) && (y >= y1) && (y <= y2))
		{
			if (i != selected_pt_idx) return i;
				else return -1;
		}
	}
	return -1;
}




