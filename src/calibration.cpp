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
	{VK_KEY_C, false},
	{VK_KEY_I, false},
	{VK_KEY_L, false},
	{VK_KEY_N, false},
	{VK_KEY_Q, false},
	{VK_KEY_S, false},
	{VK_KEY_X, false},

	{VK_KEY_UP, false},
	{VK_KEY_DOWN, false},
	{VK_KEY_LEFT, false},
	{VK_KEY_RIGHT, false},
	{VK_KEY_DEL, false}
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

const int CHESS_SIZE = 10;
const int CALIB_PT_R = 4;
const int CALIB_PT_CROSS = 8;

std::vector<calib_point> manual_calib_points;
std::vector<calib_point> intersections;
std::vector<calib_point> rule_points;

std::vector<CalibPointLine>intersections_rows;
std::vector<CalibPointLine>intersections_cols;

int selected_pt_idx = -1;

int nearest_intersection_idx = -1;
int nearest_intersection_idx_row = -1;
int nearest_intersection_idx_col = -1;

Point2f nearest_intersection;
Point2f nearest_intersection_row;
Point2f nearest_intersection_col;

const string app_folder = "/home/vevdokimov/eclipse-workspace/line_detection/Debug/";

const string calib_points_file = app_folder + "calib_points.xml";
const string intersection_points_file = app_folder + "intersections.xml";
const string intersection_points_csv_file = app_folder + "intersections_csv.csv";

bool show_cols_rows = false;
const double D2R = ((2.0 * M_PI) / 360.0);
const double R2D = (180 / M_PI);

void draw_rule_points(cv::Mat& img);
void draw_manual_calib_points(cv::Mat& img);
int get_point_quarter(Point2f pt);
void fill_intersection_counted_fields(calib_point& aPoint);
void fill_sorted_cols_rows();

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

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

double getAngle(cv::Point pt1, cv::Point pt2)
{
    double angle = atan2(pt2.y - pt1.y, pt2.x - pt1.x) * 180 / CV_PI;
    return angle;
}

Point2f getMiddle(cv::Point2f pt1, cv::Point2f pt2)
{
    return cv::Point(
        double(pt1.x + pt2.x) / 2,
        double(pt1.y + pt2.y) / 2
    );
}

double getDistance(cv::Point2f pt1, cv::Point2f pt2)
{
    double dx = pt2.x - pt1.x;
    double dy = pt2.y - pt1.y;
    return std::sqrt(dx * dx + dy * dy);
}

Point2f getIntersection(cv::Point2f A, cv::Point2f B, cv::Point2f C, cv::Point2f D)
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

int get_vector_calib_point_index(std::vector<calib_point> aVector, calib_point aPoint)
{
	int res = -1;
	for (int i = 0; i < aVector.size(); i++)
		if (aPoint.point_cnt == aVector[i].point_cnt) {
			res = i;
			break;
		}
	return res;
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
	if (intersections.size() == 0)
	{
		write_log("save_intersection_points() aborted - intersections are empty!");
		return;
	}

	std::ofstream file(intersection_points_file);
	//
	if (file.is_open())
	{
		for (size_t i = 0; i < intersections.size(); i++) {
			calib_point cp = intersections[i];
			file
				<< cp.point.x << " " << cp.point.y << " "
				<< cp.point_cnt.x << " " << cp.point_cnt.y << " "
				<< cp.col << " " << cp.row
			<< std::endl;
		}
		//
		file.close();
		write_log("save_intersection_points() successfull!");
	} else {
		write_log("save_intersection_points() file open error!");
	}
}

void load_intersection_points()
{
	std::ifstream file(intersection_points_file);
	if (!file) {
		write_log("load_intersection_points() file open error!");
		return;
	}

	intersections.clear();
	calib_point cp;

	while (file
		>> cp.point.x >> cp.point.y
		>> cp.point_cnt.x >> cp.point_cnt.y
		>> cp.col >> cp.row
	)
	{
		fill_intersection_counted_fields(cp);
		intersections.push_back(cp);
	}

	file.close();
	write_log("load_intersection_points() successfull!");

	fill_sorted_cols_rows();
}

void save_intersection_points_csv()
{
	if (intersections.size() == 0)
	{
		write_log("save_intersection_points_csv() aborted - intersections are empty!");
		return;
	}

	std::ofstream file(intersection_points_csv_file);
	//
	if (file.is_open())
	{
		file
			<< "point_cnt.x," << "point_cnt.y,"
			<< "point_mm.x," << "point_mm.y"
			<< std::endl;
		//
		for (size_t i = 0; i < intersections.size(); i++) {
			calib_point cp = intersections[i];
			file
				<< cp.point_cnt.x << "," << cp.point_cnt.y << ","
				<< cp.point_mm.x << "," << cp.point_mm.y
			<< std::endl;
		}
		//
		file.close();
		write_log("save_intersection_points_csv() successfull!");
	} else {
		write_log("save_intersection_points_csv() file open error!");
	}
}

Point2f get_point_cnt(cv::Mat& img, Point2f aPoint)
{
	cv::Point2f cnt(img.cols / 2, img.rows / 2);
	return cv::Point2f(aPoint.x - cnt.x, cnt.y - aPoint.y);
}

int col_idx, row_idx;

void fill_sorted_cols_rows()
{
	write_log("intersections.size() = " + to_string(intersections.size()));

	int min_col = 100, max_col = -100;
	int min_row = 100, max_row = -100;
	//
	for (size_t i = 0; i < intersections.size(); i++)
	{
		calib_point cp = intersections[i];
		//
		if (cp.col < min_col) min_col = cp.col;
		if (cp.col > max_col) max_col = cp.col;
		if (cp.row < min_row) min_row = cp.row;
		if (cp.row > max_row) max_row = cp.row;
	}

	//	заполняем массив сортированных столбцов
	intersections_cols.clear();
	for (col_idx = min_col; col_idx <= max_col; col_idx++)
	{
		CalibPointLine intersections_col;
		intersections_col.index = col_idx;
		std::copy_if(intersections.begin(), intersections.end(), std::back_inserter(intersections_col.points), [](calib_point ipt) {
			return ipt.col == col_idx;
		});
		std::sort(intersections_col.points.begin(), intersections_col.points.end(),
			[] (const calib_point& a, const calib_point& b) { return a.row < b.row; });
		//	расчитываем углы между точками
		for (size_t i = 0; i < intersections_col.points.size() - 1; i++)
			intersections_col.points[i].angle_col = getAngle(
				intersections_col.points[i].point,
				intersections_col.points[i + 1].point
			);
		//
		intersections_cols.push_back(intersections_col);
	}
	write_log("intersections_cols.size() = " + to_string(intersections_cols.size()));

	//	заполняем массив сортированных строк
	intersections_rows.clear();
	for (row_idx = min_row; row_idx <= max_row; row_idx++) {
		CalibPointLine intersections_row;
		intersections_row.index = row_idx;
		std::copy_if(intersections.begin(), intersections.end(), std::back_inserter(intersections_row.points), [](calib_point ipt) {
			return ipt.row == row_idx;
		});
		std::sort(intersections_row.points.begin(), intersections_row.points.end(),
			[] (const calib_point& a, const calib_point& b) { return a.col < b.col; });
		//	расчитываем углы между точками
		for (size_t i = 0; i < intersections_row.points.size() - 1; i++)
			intersections_row.points[i].angle_row = getAngle(
				intersections_row.points[i].point,
				intersections_row.points[i + 1].point
			);
		//
		intersections_rows.push_back(intersections_row);
	}
	write_log("intersections_rows.size() = " + to_string(intersections_rows.size()));
}

void fill_intersection_counted_fields(calib_point& aPoint)
{
	aPoint.point_mm = cv::Point2f(aPoint.col * CHESS_SIZE, aPoint.row * CHESS_SIZE);
	aPoint.quarter = get_point_quarter(aPoint.point_cnt);
	aPoint.angle_col = 0;
	aPoint.angle_row = 0;
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
		double angle = abs(getAngle(pt1, pt2));
		if ((angle != 0) && (angle < 50)) continue;

		Line new_line{ pt1, pt2, getMiddle(pt1, pt2), ((angle == 0) ? 1 : 2) };

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

	//	назначаем индексы вертикальным линиям
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

	//	назначаем индексы горизонтальным линиям
	index = 0;
	std::sort(lines_arr.begin(), lines_arr.end(),
			[] (const Line& a, const Line& b) { return a.mid.y < b.mid.y; });
	for (size_t i = 0; i < lines_arr.size(); i++)
		if (lines_arr[i].dir == 1) {
			if (lines_arr[i].mid == cnt_h.mid)
				cnt_h_indx = index;
			lines_arr[i].index = index++;
		}

	//	пересчитываем индексы для отсчета от центра
	for (size_t i = 0; i < lines_arr.size(); i++)
		if (lines_arr[i].dir == 1)
			lines_arr[i].index = -1 * (lines_arr[i].index - cnt_h_indx);
		else
			lines_arr[i].index -= cnt_v_indx;

	//	определяем точки пересечения линий
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
			cv::Point2f intersection = getIntersection(line1.pt1, line1.pt2, line2.pt1, line2.pt2);
			if (intersection.x < 0) continue;
			//
			calib_point new_cp;
			new_cp.point = intersection;
			new_cp.point_cnt = get_point_cnt(img, new_cp.point);
			new_cp.col = (line1.dir == 2) ? line1.index : line2.index;
			new_cp.row = (line1.dir == 1) ? line1.index : line2.index;
			//
			fill_intersection_counted_fields(new_cp);
			//
			intersections.push_back(new_cp);
		}
	}
	//
	fill_sorted_cols_rows();
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

int get_nearest_intersection_index(calib_point &pt)
{
	int idx = -1;
	double min = 10000, buf;
	//
	for (size_t i = 0; i < intersections.size(); i++)
	{
		if (pt.quarter != intersections[i].quarter) continue;
		//
		buf = getDistance(pt.point_cnt, intersections[i].point_cnt);
		if (buf < min) {
			min = buf;
			idx = i;
		}
	}
	//
	return idx;
}

CalibPointLine get_calib_point_line_by_index(std::vector<CalibPointLine> aVector, int aIndex)
{
	CalibPointLine res;
	for (size_t i = 0; i < aVector.size(); i++)
		if (aVector[i].index == aIndex)
		{
			res = aVector[i];
			break;
		}
	return res;
}

void find_point_mm(calib_point &pt)
{
	nearest_intersection_idx = get_nearest_intersection_index(pt);
	if (nearest_intersection_idx < 0) return;
	//
	calib_point base_ipt = intersections[nearest_intersection_idx];
	nearest_intersection = base_ipt.point_cnt;
	//
	std::vector<calib_point> ipt_row = get_calib_point_line_by_index(intersections_rows, base_ipt.row).points;
	std::vector<calib_point> ipt_col = get_calib_point_line_by_index(intersections_cols, base_ipt.col).points;
	//
	int dir;
	//
	//	соседняя точка в строке
	int row_idx = -1;
	for (size_t i = 0; i < ipt_row.size(); i++) {
		if (ipt_row[i].point_cnt != base_ipt.point_cnt) continue;
		//
		dir = (pt.point_cnt.x < base_ipt.point_cnt.x) ?	-1 : 1;
		if ((i + dir) < ipt_row.size()) row_idx = i + dir;
		else if ((i - dir) >= 0) row_idx = i - dir;
		//
		break;
	}
	nearest_intersection_idx_row = (row_idx > -1)
		? get_vector_calib_point_index(intersections, ipt_row[row_idx])
		: -1;
	nearest_intersection_row = intersections[nearest_intersection_idx_row].point_cnt;
	//
	//	соседняя точка в столбце
	int col_idx = -1;
	for (size_t i = 0; i < ipt_col.size(); i++) {
		if (ipt_col[i].point_cnt != base_ipt.point_cnt) continue;
		//
		dir = (pt.point_cnt.y < base_ipt.point_cnt.y) ?	-1 : 1;
		if ((i + dir) < ipt_col.size()) col_idx = i + dir;
		else if ((i - dir) >= 0) col_idx = i - dir;
		//
		break;
	}
	nearest_intersection_idx_col = (col_idx > -1)
		? get_vector_calib_point_index(intersections, ipt_col[col_idx])
		: -1;
	nearest_intersection_col = intersections[nearest_intersection_idx_col].point_cnt;
	//
	if ((nearest_intersection_idx_row > -1) &&
		(nearest_intersection_idx_col > -1)) {
		//
		calib_point row_ipt = intersections[nearest_intersection_idx_row];
		double row_k = getDistance(base_ipt.point_mm, row_ipt.point_mm) / getDistance(base_ipt.point_cnt, row_ipt.point_cnt);
		calib_point col_ipt = intersections[nearest_intersection_idx_col];
		double col_k = getDistance(base_ipt.point_mm, col_ipt.point_mm) / getDistance(base_ipt.point_cnt, col_ipt.point_cnt);
		//
		if (base_ipt.point_cnt.x > row_ipt.point_cnt.x)
			pt.point_mm.x = row_ipt.point_mm.x + (pt.point_cnt.x - row_ipt.point_cnt.x) * row_k;
		else
			pt.point_mm.x = base_ipt.point_mm.x + (pt.point_cnt.x - base_ipt.point_cnt.x) * row_k;
		//
		if (base_ipt.point_cnt.y > col_ipt.point_cnt.y)
			pt.point_mm.y = col_ipt.point_mm.y + (pt.point_cnt.y - col_ipt.point_cnt.y) * col_k;
		else
			pt.point_mm.y = base_ipt.point_mm.y + (pt.point_cnt.y - base_ipt.point_cnt.y) * col_k;
	}
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
    	if (manual_calib_points.size())
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
	if (is_key_on(VK_KEY_C))
	{
		toggle_key(VK_KEY_C);
		save_intersection_points_csv();
	}
	//
	if (is_key_on(VK_KEY_N))
	{
		toggle_key(VK_KEY_N);
		load_intersection_points();
	}
	//
	if (is_key_on(VK_KEY_L))
	{
		toggle_key(VK_KEY_L);
		write_log("manual_calib_points load");
		//
		load_manual_calib_points();
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
		write_log("manual_calib_point " + to_string(selected_pt_idx) +  " delete");
		//
		manual_calib_points.erase(manual_calib_points.begin() + selected_pt_idx);
		selected_pt_idx = -1;
	}
	//
	if (is_key_on(VK_KEY_UP))
	{
		toggle_key(VK_KEY_UP);
		if (selected_pt_idx == -1) return;
		write_log("manual_calib_point " + to_string(selected_pt_idx) +  " up");
		//
		manual_calib_points[selected_pt_idx].point.y -= 1;
	}
	//
	if (is_key_on(VK_KEY_DOWN))
	{
		toggle_key(VK_KEY_DOWN);
		if (selected_pt_idx == -1) return;
		write_log("manual_calib_point " + to_string(selected_pt_idx) +  " down");
		//
		manual_calib_points[selected_pt_idx].point.y += 1;
	}
	//
	if (is_key_on(VK_KEY_LEFT))
	{
		toggle_key(VK_KEY_LEFT);
		if (selected_pt_idx == -1) return;
		write_log("manual_calib_point " + to_string(selected_pt_idx) +  " left");
		//
		manual_calib_points[selected_pt_idx].point.x -= 1;
	}
	//
	if (is_key_on(VK_KEY_RIGHT))
	{
		toggle_key(VK_KEY_RIGHT);
		if (selected_pt_idx == -1) return;
		write_log("manual_calib_point " + to_string(selected_pt_idx) +  " right");
		//
		manual_calib_points[selected_pt_idx].point.x += 1;
	}
}

void draw_intersection_points(cv::Mat& img)
{
	int fontFace = 1;
	double fontScale = 0.5;
	Scalar fontColor = CLR_GREEN;	//	CLR_RED;	CLR_GREEN;
	int circleRadius = CALIB_PT_R;	//	4;
	//
	bool show_cols_rows = false;
	//
	for (size_t i = 0; i < intersections_cols.size(); i++)
	{
		for (size_t j = 0; j < intersections_cols[i].points.size(); j++)
		{
			calib_point cp = intersections_cols[i].points[j];
			//
			bool fl1 = false, fl2 = false, fl3 = false;
			if (cp.point_cnt == nearest_intersection)
				fl1 = true;
			if (cp.point_cnt == nearest_intersection_col)
				fl2 = true;
			if (cp.point_cnt == nearest_intersection_row)
				fl3 = true;
			Scalar clr = (fl1 || fl2 || fl3)
				? CLR_MAGENTA
				: CLR_RED;
			//
			cv::circle(img, cp.point, circleRadius, clr, 1, cv::LINE_AA);
			if (cp.point_cnt == nearest_intersection) {
				cv::line(img,
					cp.point - cv::Point2f(0, CALIB_PT_CROSS),
					cp.point + cv::Point2f(0, CALIB_PT_CROSS),
					clr, 1, cv::LINE_AA, 0);
				cv::line(img,
					cp.point - cv::Point2f(CALIB_PT_CROSS, 0),
					cp.point + cv::Point2f(CALIB_PT_CROSS, 0),
					clr, 1, cv::LINE_AA, 0);
			}
//			putText(img,
//					to_string((int)round(cp.angle_col)),
//					cp.point + cv::Point2f(2, 12),
//					fontFace, fontScale, fontColor);
			//
//			putText(img,
//				show_cols_rows ? to_string(cp.col) : to_string((int)cp.point_cnt.x),
//				cp.point + cv::Point2f(5, 8),
//				fontFace, fontScale, fontColor);
//			putText(img,
//				show_cols_rows ? to_string(cp.row) : to_string((int)cp.point_cnt.y),
//				cp.point + cv::Point2f(5, 16),
//				fontFace, fontScale, fontColor);
		}
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
		cv::line(img,
			mcp.point - cv::Point2f(0, CALIB_PT_CROSS),
			mcp.point + cv::Point2f(0, CALIB_PT_CROSS),
			clr, 1, cv::LINE_AA, 0);
		cv::line(img,
			mcp.point - cv::Point2f(CALIB_PT_CROSS, 0),
			mcp.point + cv::Point2f(CALIB_PT_CROSS, 0),
			clr, 1, cv::LINE_AA, 0);
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
	const double fontScale = 0.8;
	const int offsetY = 10;
	//
	int r;
	//
	for (size_t i = 0; i < rule_points.size(); i++)
	{
		r = 1;
		calib_point pt = rule_points[i];
		cv::circle(img, rule_points[i].point, CALIB_PT_R, clr, 1, cv::LINE_AA);
		cv::line(img,
			pt.point - cv::Point2f(CALIB_PT_CROSS, 0),
			pt.point + cv::Point2f(CALIB_PT_CROSS, 0),
			clr, 1, cv::LINE_AA, 0);
		cv::line(img,
			pt.point - cv::Point2f(0, CALIB_PT_CROSS),
			pt.point + cv::Point2f(0, CALIB_PT_CROSS),
			clr, 1, cv::LINE_AA, 0);
		//
		putText(img,
			string_format("%.3f", pt.point_mm.x),
			//((int)round(pt.point_mm.x)),
			pt.point + cv::Point2f(5, offsetY * r++),
			1, fontScale, clr);
		putText(img,
			string_format("%.3f", pt.point_mm.y),
			//((int)round(pt.point_mm.y)),
			pt.point + cv::Point2f(5, offsetY * r++),
			1, fontScale, clr);
	}
	//
	if (rule_points.size() == 2)
	{
		cv::line(img, rule_points[0].point, rule_points[1].point, clr, 1, cv::LINE_AA, 0);
		double dist = getDistance(rule_points[0].point_mm, rule_points[1].point_mm);
		putText(img,
			string_format("L=%.3f", dist),
			//to_string((int)round(dist)),
			rule_points[1].point + cv::Point2f(5, offsetY * r++),
			1, fontScale, CLR_MAGENTA
		);
	}
}
