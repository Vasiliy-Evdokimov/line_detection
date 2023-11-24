/*
 * calibration.cpp
 *
 *  Created on: 2 нояб. 2023 г.
 *      Author: vevdokimov
 */

#include <fstream>

#include "calibration.hpp"

using namespace std;

std::map<int, bool> keys_toggle = {
	{VK_KEY_A, false},
	{VK_KEY_B, false},
	{VK_KEY_I, false},
	{VK_KEY_L, false},
	{VK_KEY_Q, false},
	{VK_KEY_S, false},
	{VK_KEY_X, false},

	{VK_KEY_UP, false},
	{VK_KEY_DOWN, false},
	{VK_KEY_LEFT, false},
	{VK_KEY_RIGHT, false},
	{VK_KEY_DEL, false}
};

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

const int CHESS_SIZE = 10;

std::vector<calib_point> manual_calib_points;
std::vector<calib_point> intersections;
std::vector<calib_point> rule_points;

int selected_pt_idx = -1;
int nearest_intersection_idx = -1;

const int CALIB_PT_R = 6;

const string app_folder = "/home/vevdokimov/eclipse-workspace/line_detection/Debug/";

const string calib_points_file = app_folder + "calib_points.xml";

const double D2R = ((2.0 * M_PI) / 360.0);
const double R2D = (180 / M_PI);

void draw_rule_points(cv::Mat& img);
void draw_manual_calib_points(cv::Mat& img);
int get_point_quarter(Point2f pt);

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

double findAngle(cv::Point pt1, cv::Point pt2)
{
    double angle = atan2(pt2.y - pt1.y, pt2.x - pt1.x) * 180 / CV_PI;
    return angle;
}

Point2f middle(cv::Point2f pt1, cv::Point2f pt2)
{
    return cv::Point(
        double(pt1.x + pt2.x) / 2,
        double(pt1.y + pt2.y) / 2
    );
}

double distance(cv::Point2f pt1, cv::Point2f pt2)
{
    double dx = pt2.x - pt1.x;
    double dy = pt2.y - pt1.y;
    return std::sqrt(dx * dx + dy * dy);
}

Point2f findIntersection(cv::Point2f A, cv::Point2f B, cv::Point2f C, cv::Point2f D)
{
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

void recount_cols_rows(cv::Point2f center)
{
	const int MAX_OFFSET = 20;

	std::sort(manual_calib_points.begin(), manual_calib_points.end(),
		[] (const calib_point& a, const calib_point& b) { return a.point.x < b.point.x; });

	for (size_t i = 1; i < manual_calib_points.size(); i++)
		manual_calib_points[i].col =
			(abs(manual_calib_points[i].point.x - manual_calib_points[i - 1].point.x) > MAX_OFFSET)
			? manual_calib_points[i - 1].col + 1
			: manual_calib_points[i - 1].col;

	std::sort(manual_calib_points.begin(), manual_calib_points.end(),
		[] (const calib_point& a, const calib_point& b) { return a.point.y > b.point.y; });

	for (size_t i = 1; i < manual_calib_points.size(); i++)
		manual_calib_points[i].row =
			(abs(manual_calib_points[i].point.y - manual_calib_points[i - 1].point.y) > MAX_OFFSET)
			? manual_calib_points[i - 1].row + 1
			: manual_calib_points[i - 1].row;
}

void save_manual_calib_points()
{
	std::vector<cv::Point2f> calib_points_for_file;
	//
	for (size_t i = 0; i < manual_calib_points.size(); i++)
		calib_points_for_file.push_back(manual_calib_points[i].point);
	//
	cv::FileStorage file(calib_points_file, cv::FileStorage::WRITE);
	file << "calibPoints" << calib_points_for_file;
	file.release();
}

void load_manual_calib_points()
{
	std::vector<cv::Point2f> calib_points_for_file;
	cv::FileStorage file(calib_points_file, cv::FileStorage::READ);
	file["calibPoints"] >> calib_points_for_file;
	file.release();
	//
	manual_calib_points.clear();
	for (size_t i = 0; i < calib_points_for_file.size(); i++)
		manual_calib_points.push_back({
			calib_points_for_file[i],
			cv::Point2f(0, 0),
			cv::Point2f(0, 0),
			0, 0
		});
}

void save_intersection_points()
{
	const string calib_points_file = app_folder + "intersections.csv";
	//
	std::ofstream out;
	out.open(calib_points_file);
	//
	if (out.is_open())
	{
		for (size_t i = 0; i < intersections.size() - 1; i++)
			out	<< (int)intersections[i].point_mm.x << ", "
				<< (int)intersections[i].point_mm.y << ", "
				<< (int)intersections[i].point_cnt.x << ", "
				<< (int)intersections[i].point_cnt.y << std::endl;
	}
	out.close();
	std::cout << "Intersections file has been written!" << std::endl;
}

Point2f get_point_cnt(cv::Mat& img, Point2f aPoint)
{
	cv::Point2f cnt(img.cols / 2, img.rows / 2);
	return cv::Point2f(aPoint.x - cnt.x, cnt.y - aPoint.y);
}

void fill_intersection_points(cv::Mat& img)
{
	const int CANNY_LOW = 50;	//	50;		//	250;	//  нижняя граница распознавания контуров
	const int CANNY_HIGH = 150; //	150; 	//	350;	//  верхняя     -//-
	const int HOUGH_LEVEL = 90; // 	150 	//	30;		//  уровень распознавания прямых линий на контурах

	cv::Point cnt(img.cols / 2, img.rows / 2);

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

		//	если угол между точками не 0 (горизорнитальная линия) или меньше 50 - не вертикальная,
		//	то пропускаем эту линию
		double angle = abs(findAngle(pt1, pt2));
		if ((angle != 0) && (angle < 50)) continue;

		Line new_line{ pt1, pt2, middle(pt1, pt2), ((angle == 0) ? 1 : 2) };

		//	если в списке итоговых линий есть линия, отстоящая от проверяемой менее, чем на mid_delta,
		//	то не добавляем линию в результат
		const int mid_delta = 10;
		bool fl = true;
		for (size_t j = 0; j < lines_arr.size(); j++)
			if (new_line.dir == lines_arr[j].dir) {
				if (((new_line.dir == 1) and (abs(new_line.mid.y - lines_arr[j].mid.y) < mid_delta)) ||
					((new_line.dir == 2) and (abs(new_line.mid.x - lines_arr[j].mid.x) < mid_delta)))
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

	intersections.clear();
	//
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
			cp.point_cnt = get_point_cnt(img, cp.point);
			cp.col = (line1.dir == 2) ? line1.index : line2.index;
			cp.row = (line1.dir == 1) ? line1.index : line2.index;
			cp.point_mm = cv::Point2f(cp.col * CHESS_SIZE, cp.row * CHESS_SIZE);
			cp.quarter = get_point_quarter(cp.point_cnt);
			intersections.push_back(cp);
		}
	}
	//
	write_log("intersections.size() = " + to_string(intersections.size()));
}

int get_point_quarter(Point2f pt)
{
	int quarter = 0;
	if ((pt.x >=0) && (pt.y >=0))
		quarter = 1;
	else if ((pt.x < 0) && (pt.y >=0))
		quarter = 2;
	else if ((pt.x < 0) && (pt.y < 0))
		quarter = 3;
	else if ((pt.x > 0) && (pt.y < 0))
		quarter = 4;
	return quarter;
}

int get_nearest_inbtersection_index(calib_point &pt)
{
	int idx = -1;
	double min = 10000, buf;
	//
	for (size_t i = 0; i < intersections.size(); i++)
	{
		if (pt.quarter != intersections[i].quarter) continue;
		//
		buf = distance(pt.point_cnt, intersections[i].point_cnt);
		if (buf < min) {
			min = buf;
			idx = i;
		}
	}
	//
	return idx;
}

void find_point_mm(calib_point &pt)
{
	nearest_intersection_idx = get_nearest_inbtersection_index(pt);
}

void onMouse(int event, int x, int y, int flags, void* userdata)
{
	cv::Mat* img = (cv::Mat*)userdata;
	cv::Point2f pt =  cv::Point2f(x, y);
	cv::Point2f pt_cnt = get_point_cnt(*img, pt);

	if (event == cv::EVENT_LBUTTONDOWN)
    {
        std::cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << std::endl;
        //
        if (is_key_on(VK_KEY_A))
        {
        	selected_pt_idx = select_calib_pt(x, y);
        }
        //
        else if (is_key_on(VK_KEY_Q))
        {
        	if (rule_points.size() == 2) rule_points.clear();
        	//
        	rule_points.push_back({
        		pt,
				pt_cnt,
				cv::Point(0, 0),
				0, 0,
				get_point_quarter(pt_cnt)
        	});
        	//
        	find_point_mm(rule_points[rule_points.size() - 1]);
        }
        //
        else
        {
        	manual_calib_points.push_back({
        		pt,
				pt_cnt,
				cv::Point(0, 0),
				0, 0,
				get_point_quarter(pt_cnt)
        	});
        }
    }
    else if (event == cv::EVENT_RBUTTONDOWN)
    {
        //	std::cout << "Right button of the mouse is clicked - position (" << x << ", " << y << ")" << std::endl;
    }
    else if (event == cv::EVENT_MBUTTONDOWN)
    {
    	//	std::cout << "Middle button of the mouse is clicked - position (" << x << ", " << y << ")" << std::endl;
    	manual_calib_points.erase(manual_calib_points.end() - 1);
    	//calib_points.clear();
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
	if (is_key_on(VK_KEY_I))
	{
		toggle_key(VK_KEY_I);
		fill_intersection_points(img);
		write_log("intersection points filled!");
	}
	//
	if (is_key_on(VK_KEY_B))
	{
		toggle_key(VK_KEY_B);
		save_intersection_points();
	}
	//
	if (is_key_on(VK_KEY_L))
	{
		toggle_key(VK_KEY_L);
		write_log("manual_calib_points load");
		//
		load_manual_calib_points();
		recount_cols_rows(Point2f(img.cols / 2, img.rows / 2));
	}
	//
	if (is_key_on(VK_KEY_S))
	{
		toggle_key(VK_KEY_S);
		write_log("manual_calib_points save");
		//
		save_manual_calib_points();
	}
	//
	if (is_key_on(VK_KEY_X))
	{
		toggle_key(VK_KEY_X);
		write_log("manual_calib_points clear");
		//
		manual_calib_points.clear();
	}
	//
	if (is_key_on(VK_KEY_DEL))
	{
		toggle_key(VK_KEY_DEL);
		if (selected_pt_idx == -1) return;
		write_log("point " + to_string(selected_pt_idx) +  " delete");
		//
		manual_calib_points.erase(manual_calib_points.begin() + selected_pt_idx);
		selected_pt_idx = -1;
	}
	//
	if (is_key_on(VK_KEY_UP))
	{
		toggle_key(VK_KEY_UP);
		if (selected_pt_idx == -1) return;
		write_log("point " + to_string(selected_pt_idx) +  " up");
		//
		manual_calib_points[selected_pt_idx].point.y -= 1;
	}
	//
	if (is_key_on(VK_KEY_DOWN))
	{
		toggle_key(VK_KEY_DOWN);
		if (selected_pt_idx == -1) return;
		write_log("point " + to_string(selected_pt_idx) +  " down");
		//
		manual_calib_points[selected_pt_idx].point.y += 1;
	}
	//
	if (is_key_on(VK_KEY_LEFT))
	{
		toggle_key(VK_KEY_LEFT);
		if (selected_pt_idx == -1) return;
		write_log("point " + to_string(selected_pt_idx) +  " left");
		//
		manual_calib_points[selected_pt_idx].point.x -= 1;
	}
	//
	if (is_key_on(VK_KEY_RIGHT))
	{
		toggle_key(VK_KEY_RIGHT);
		if (selected_pt_idx == -1) return;
		write_log("point " + to_string(selected_pt_idx) +  " right");
		//
		manual_calib_points[selected_pt_idx].point.x += 1;
	}
}

void draw_intersection_points(cv::Mat& img)
{
	int fontFace = 1;
	double fontScale = 0.5;
	Scalar fontColor = CLR_GREEN;	//	CLR_RED;	CLR_GREEN;
	int circleRadius = 4;
	//
	bool show_cols_rows = false;
	//
	cv::Point cnt(img.cols / 2, img.rows / 2);
	//
	for (size_t i = 0; i < intersections.size(); i++)
	{
		calib_point cp = intersections[i];
		Scalar clr = (i == nearest_intersection_idx) ? CLR_MAGENTA : CLR_RED;
		cv::circle(img, cp.point, circleRadius, clr, 1, cv::LINE_AA);
//		putText(img,
//			to_string(cp.col) + ";" + to_string(cp.row),
//			cp.point + cv::Point2f(3, ((cp.col % 2 == 0) ? 8 : 16)),
//			fontFace, fontScale, fontColor);
		putText(img,
			show_cols_rows ? to_string(cp.col) : to_string((int)cp.point_cnt.x),
			cp.point + cv::Point2f(5, 8),
			fontFace, fontScale, fontColor);
		putText(img,
			show_cols_rows ? to_string(cp.row) : to_string((int)cp.point_cnt.y),
			cp.point + cv::Point2f(5, 16),
			fontFace, fontScale, fontColor);
	}
}

void calib_points(cv::Mat& img)
{
	cv::Point cnt(img.cols / 2, img.rows / 2);
	//
	handle_keys(img);
	//
	draw_intersection_points(img);
	draw_manual_calib_points(img);
	draw_rule_points(img);
	//
	cv::line(img, cv::Point2f(cnt.x, 0), cv::Point2f(cnt.x, img.rows),
		CLR_YELLOW, 1, cv::LINE_AA, 0);
	cv::line(img, cv::Point2f(0, cnt.y), cv::Point2f(img.cols, cnt.y),
		CLR_YELLOW, 1, cv::LINE_AA, 0);
	//
	if (is_key_on(VK_KEY_A))
		putText(img, "SELECTION MODE", cv::Point(5, 20), 1, 1.5, CLR_MAGENTA, 2);
	if (is_key_on(VK_KEY_Q))
		putText(img, "RULE MODE", cv::Point(5, 40), 1, 1.5, CLR_MAGENTA, 2);
	else if (rule_points.size() > 0) rule_points.clear();
	//
//	if ((false) && (calib_points.size() == 2)) {
//		cv::line(img, calib_points[0].point, calib_points[1].point, CLR_RED, 1, cv::LINE_AA, 0);
//		//
//		double dist_fov = -1180;
//		//
//		cv::Point2f pt0, pt1;
//		pundistors(pt0, calib_points[0].point, img.cols, img.rows, dist_fov);
//		pundistors(pt1, calib_points[1].point, img.cols, img.rows, dist_fov);
//		//
//		double dist1 = GetPointDist(calib_points[0].point, calib_points[1].point);
//		double dist2 = GetPointDist(pt0, pt1);
//		int y = 1, off = 15;
//		//putText(img, to_string(dist1) + "px", calib_points[0] + cv::Point2f(10, off * y++), 1, 1, CLR_RED);
//		putText(img, to_string(dist2) + "mm", calib_points[0].point + cv::Point2f(10, off * y++), 1, 1, CLR_RED);
//		//putText(img, to_string(calib_points[0].y) + "y", calib_points[0] + cv::Point2f(10, off * y++), 1, 1, CLR_RED);
//		putText(img, to_string(24 / dist2) + "d", calib_points[0].point + cv::Point2f(10, off * y++), 1, 1, CLR_RED);
//	}
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

void draw_manual_calib_points(cv::Mat& img)
{
	cv::Scalar clr;
	for (size_t i = 0; i < manual_calib_points.size(); i++)
	{
		clr = (i != selected_pt_idx) ? CLR_RED : CLR_GREEN;
		calib_point mcp = manual_calib_points[i];
		//
		cv::circle(img, mcp.point, CALIB_PT_R, clr, 1, cv::LINE_AA);
		cv::line(img, mcp.point - cv::Point2f(0, 10), mcp.point + cv::Point2f(0, 10), clr, 1, cv::LINE_AA, 0);
		cv::line(img, mcp.point - cv::Point2f(10, 0), mcp.point + cv::Point2f(10, 0), clr, 1, cv::LINE_AA, 0);
		putText(img, to_string(mcp.col) + ";" + to_string(mcp.row),
				mcp.point + cv::Point2f(10, 10), 1, 1, CLR_RED);
	}
}

int select_calib_pt(int x, int y)
{
	calib_point mcp;
	for (size_t i = 0; i < manual_calib_points.size(); i++)
	{
		mcp = manual_calib_points[i];
		int x1 = mcp.point.x - CALIB_PT_R;
		int x2 = mcp.point.x + CALIB_PT_R;
		int y1 = mcp.point.y - CALIB_PT_R;
		int y2 = mcp.point.y + CALIB_PT_R;
		//
		if ((x >= x1) && ( x <= x2) && (y >= y1) && (y <= y2))
		{
			if (i != selected_pt_idx) return i;
				else return -1;
		}
	}
	return -1;
}

void draw_rule_points(cv::Mat& img)
{
	cv::Scalar clr = CLR_GREEN;
	//
	for (size_t i = 0; i < rule_points.size(); i++)
	{
		cv::circle(img, rule_points[i].point, CALIB_PT_R, clr, 1, cv::LINE_AA);
		cv::line(img,
			rule_points[i].point - cv::Point2f(10, 0),
			rule_points[i].point + cv::Point2f(10, 0),
			clr, 1, cv::LINE_AA, 0);
		cv::line(img,
			rule_points[i].point - cv::Point2f(0, 10),
			rule_points[i].point + cv::Point2f(0, 10),
			clr, 1, cv::LINE_AA, 0);
	}
	//
	if (rule_points.size() == 2)
		cv::line(img, rule_points[0].point, rule_points[1].point, clr, 1, cv::LINE_AA, 0);
}
