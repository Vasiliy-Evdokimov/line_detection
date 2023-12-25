/*
 * calibration.cpp
 *
 *  Created on: 2 нояб. 2023 г.
 *      Author: vevdokimov
 */

#include <fstream>
#include <cmath>

#include "defines.hpp"
#include "config_path.hpp"
#include "calibration.hpp"

using namespace std;

std::map<int, bool> keys_toggle =
{
	{ VK_KEY_A, false },
	{ VK_KEY_B, false },
	{ VK_KEY_C, false },
	{ VK_KEY_D, false },
	{ VK_KEY_F, false },
	{ VK_KEY_I, false },
	{ VK_KEY_L, false },
	{ VK_KEY_M, false },
	{ VK_KEY_N, false },
	{ VK_KEY_Q, false },
	{ VK_KEY_S, false },
	{ VK_KEY_W, false },
	{ VK_KEY_X, false },

	{ VK_KEY_UP, false },
	{ VK_KEY_DOWN, false },
	{ VK_KEY_LEFT, false },
	{ VK_KEY_RIGHT, false },
	{ VK_KEY_DEL, false }
};

std::map<int, string> modes_list =
{
	{ MODE_NOT_SELECTED, "NOT_SELECTED" },
	{ MODE_SELECT_LINE, "SELECT_LINE" },
	{ MODE_SELECT_POINT, "SELECT_POINT" },
	{ MODE_ADD_USER_LINE, "ADD_USER_LINE" },
	{ MODE_ADD_USER_POINT, "ADD_USER_POINT" },
	{ MODE_RULER, "RULER" }
};

cv::Mat cameraMatrix;
cv::Mat distCoeffs;

std::set<int> current_modes;

const int CHESS_SIZE = 10;
const int CALIB_PT_R = 4;
const int CALIB_PT_CROSS = 8;

std::vector<CalibPoint> intersections;
std::vector<CalibPoint> rule_points;
std::vector<Point2f> new_line_points;

std::vector<CalibPointLine>intersections_rows;
std::vector<CalibPointLine>intersections_cols;

int selected_idx = -1;

int nearest_intersection_idx = -1;
int nearest_intersection_idx_row = -1;
int nearest_intersection_idx_col = -1;

Point2f nearest_intersection;
Point2f nearest_intersection_row;
Point2f nearest_intersection_col;

const string calibration_filename = "calibration.xml";

const string intersection_points_filename = "intersections.xml";
const string intersection_lines_filename = "intersections_lines.xml";
const string intersection_points_csv_filename = "intersections_csv.csv";

bool show_cols_rows = false;

std::vector<Line> intersections_lines;

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

double getDistanceToLine(Point2f pt, Point2f pt1, Point2f pt2)
{
    double numerator = abs((pt2.y - pt1.y) * pt.x - (pt2.x - pt1.x) * pt.y + pt2.x * pt1.y - pt2.y * pt1.x);
    double denominator = sqrt(pow(pt2.y - pt1.y, 2) + pow(pt2.x - pt1.x, 2));
    return numerator / denominator;
}

int get_vector_calib_point_index(std::vector<CalibPoint> aVector, CalibPoint aPoint)
{
	int res = -1;
	for (int i = 0; i < aVector.size(); i++)
		if (aPoint.point_cnt == aVector[i].point_cnt) {
			res = i;
			break;
		}
	return res;
}

void read_calibration()
{
	string calibration_file_path =
		get_actual_config_directory() +	calibration_filename;
	write_log("calibration_file_path = " + calibration_file_path);

	cv::FileStorage fs(calibration_file_path, cv::FileStorage::READ);

	fs["cameraMatrix"] >> cameraMatrix;
	fs["distCoeffs"] >> distCoeffs;

	fs.release();
}

void save_intersection_points()
{
	if (intersections.size() == 0)
	{
		write_log("save_intersection_points() aborted - intersections are empty!");
		return;
	}

	string intersection_points_file_path =
		get_actual_config_directory() + intersection_points_filename;
	write_log("intersection_points_file_path = " + intersection_points_file_path);

	std::ofstream file(intersection_points_file_path);
	//
	if (file.is_open())
	{
		for (size_t i = 0; i < intersections.size(); i++) {
			CalibPoint cp = intersections[i];
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

void save_intersection_lines()
{
	if (intersections_lines.size() == 0)
	{
		write_log("save_intersection_lines() aborted - intersections_lines are empty!");
		return;
	}

	string intersection_lines_file_path =
		get_actual_config_directory() +	intersection_lines_filename;
	write_log("intersection_lines_file_path = " + intersection_lines_file_path);

	std::ofstream file(intersection_lines_file_path);
	//
	if (file.is_open())
	{
		for (size_t i = 0; i < intersections_lines.size(); i++) {
			Line line = intersections_lines[i];
			file
				<< line.pt1.x << " " << line.pt1.y << " "
				<< line.pt2.x << " " << line.pt2.y << " "
				<< line.mid.x << " " << line.mid.y << " "
				<< line.dir << " "
				<< line.index << " "
				<< line.type << std::endl;
		}
		//
		file.close();
		write_log("save_intersection_lines() successfull!");
	} else {
		write_log("save_intersection_lines() file open error!");
	}
}

void load_intersection_points()
{
	string intersection_points_file_path =
		get_actual_config_directory() +	intersection_points_filename;
	write_log("intersection_points_file_path = " + intersection_points_file_path);

	std::ifstream file(intersection_points_file_path);
	if (!file) {
		write_log("load_intersection_points() file open error!");
		return;
	}

	intersections.clear();
	CalibPoint cp;

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

void load_intersection_lines()
{
	string intersection_lines_file_path =
		get_actual_config_directory() +	intersection_lines_filename;
	write_log("intersection_lines_file_path = " + intersection_lines_file_path);

	std::ifstream file(intersection_lines_file_path);
	if (!file) {
		write_log("load_intersection_lines() file open error!");
		return;
	}

	intersections_lines.clear();
	Line line;

	while (file
		>> line.pt1.x >> line.pt1.y
		>> line.pt2.x >> line.pt2.y
		>> line.mid.x >> line.mid.y
		>> line.dir
		>> line.index
		>> line.mid.y
	)
	{
		intersections_lines.push_back(line);
	}

	file.close();
	write_log("load_intersection_lines() successfull!");
}

void save_intersection_points_csv()
{
	if (intersections.size() == 0)
	{
		write_log("save_intersection_points_csv() aborted - intersections are empty!");
		return;
	}

	string intersection_points_csv_file_path =
		get_actual_config_directory() + intersection_points_csv_filename;
	write_log("intersection_points_csv_file_path = " + intersection_points_csv_file_path);

	std::ofstream file(intersection_points_csv_file_path);
	//
	if (file.is_open())
	{
		file
			<< "point_cnt.x," << "point_cnt.y,"
			<< "point_mm.x," << "point_mm.y"
			<< std::endl;
		//
		for (size_t i = 0; i < intersections.size(); i++) {
			CalibPoint cp = intersections[i];
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

Point2f get_point_cnt(cv::Mat img, Point2f aPoint)
{
	cv::Point2f cnt(img.cols / 2, img.rows / 2);
	return cv::Point2f(aPoint.x - cnt.x, cnt.y - aPoint.y);
}

Point2f point_cnt_to_topleft(cv::Mat img, Point2f aPoint)
{
	cv::Point2f cnt(img.cols / 2, img.rows / 2);
	return cv::Point2f(aPoint.x + cnt.x, cnt.y - aPoint.y);
}

CalibPoint get_calib_point(cv::Mat img, cv::Point2f pt)
{
	CalibPoint res{ pt, get_point_cnt(img, pt) };
	find_point_mm(res);
	return res;
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
		CalibPoint cp = intersections[i];
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
		std::copy_if(intersections.begin(), intersections.end(), std::back_inserter(intersections_col.points), [](CalibPoint ipt) {
			return ipt.col == col_idx;
		});
		std::sort(intersections_col.points.begin(), intersections_col.points.end(),
			[] (const CalibPoint& a, const CalibPoint& b) { return a.row < b.row; });
		//
		intersections_cols.push_back(intersections_col);
	}
	write_log("intersections_cols.size() = " + to_string(intersections_cols.size()));

	//	заполняем массив сортированных строк
	intersections_rows.clear();
	for (row_idx = min_row; row_idx <= max_row; row_idx++) {
		CalibPointLine intersections_row;
		intersections_row.index = row_idx;
		std::copy_if(intersections.begin(), intersections.end(), std::back_inserter(intersections_row.points), [](CalibPoint ipt) {
			return ipt.row == row_idx;
		});
		std::sort(intersections_row.points.begin(), intersections_row.points.end(),
			[] (const CalibPoint& a, const CalibPoint& b) { return a.col < b.col; });
		//
		intersections_rows.push_back(intersections_row);
	}
	write_log("intersections_rows.size() = " + to_string(intersections_rows.size()));
}

void recount_center_points(cv::Mat img)
{
	// ищем нулевую точку
	auto result{ std::find_if(intersections.begin(), intersections.end(), [](CalibPoint ipt)
			{ return (ipt.point_cnt.x == 0) && (ipt.point_cnt.y == 0); }) };
	// если не нашли - добавляем
	if (result == intersections.end())
		intersections.push_back({
			cv::Point(img.cols / 2, img.rows / 2),
			cv::Point(0, 0),
			cv::Point(0, 0),
			0, 0
		});
	// сортируем все точки по возрастанию X
	std::sort(intersections.begin(), intersections.end(),
		[] (const CalibPoint& a, const CalibPoint& b) { return a.point_cnt.x < b.point_cnt.x; });
	// находим индекс нулевой точки
	auto result2{ std::find_if(intersections.begin(), intersections.end(), [](CalibPoint ipt)
			{ return (ipt.point_cnt.x == 0) && (ipt.point_cnt.y == 0); }) };
	int zero_idx = (result2 - intersections.begin());
	//
	for (int i = zero_idx - 1, j = -1; i >= 0; i--, j--)
		intersections[i].col = j;
	for (int i = zero_idx + 1, j = 1; i < intersections.size(); i++, j++)
		intersections[i].col = j;
	for (int i = 0; i < intersections.size(); i++)
	{
		intersections[i].row = 0;
		intersections[i].point_mm.x = intersections[i].col * CHESS_SIZE;
		intersections[i].point_mm.y = 0;
	}
}

void fill_intersection_counted_fields(CalibPoint& aPoint)
{
	aPoint.point_mm = cv::Point2f(aPoint.col * CHESS_SIZE, aPoint.row * CHESS_SIZE);
}

void fill_opencv_intersections_lines(cv::Mat& img)
{
	intersections_lines.clear();

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
	//	cv::imshow("Canny Edges", edges);
	//
	std::vector<cv::Vec2f> lines;
	cv::HoughLines(edges, lines, 1, CV_PI / 180, HOUGH_LEVEL, 0, 0);

	Line cnt_h{Point2f(0, cnt.y), Point2f(img.cols, cnt.y), cnt, 1};
	intersections_lines.push_back(cnt_h);
	Line cnt_v{Point2f(cnt.x, 0), Point2f(cnt.x, img.rows), cnt, 2};
	intersections_lines.push_back(cnt_v);

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

		Line new_line{ pt1, pt2, getMiddle(pt1, pt2), ((angle == 0) ? 1 : 2), 0, 1 };

		//	если в списке итоговых линий есть линия, отстоящая от проверяемой менее, чем на mid_delta,
		//	то не добавляем линию в результат
		const int mid_delta = 10;
		bool fl = true;
		for (size_t j = 0; j < intersections_lines.size(); j++)
			if (new_line.dir == intersections_lines[j].dir) {
				if (((new_line.dir == 1) and (abs(new_line.mid.y - intersections_lines[j].mid.y) < mid_delta)) ||
					((new_line.dir == 2) and (abs(new_line.mid.x - intersections_lines[j].mid.x) < mid_delta)))
				{
					fl = false;
					break;
				}
			}
		if (fl) intersections_lines.push_back(new_line);
	}
}

void fill_intersection_points(cv::Mat& img)
{
	cv::Point cnt(img.cols / 2, img.rows / 2);
	Line cnt_h{Point2f(0, cnt.y), Point2f(img.cols, cnt.y), cnt, 1};
	Line cnt_v{Point2f(cnt.x, 0), Point2f(cnt.x, img.rows), cnt, 2};

	//	назначаем индексы вертикальным линиям
	int index = 0;
	int cnt_h_indx, cnt_v_indx;
	std::sort(intersections_lines.begin(), intersections_lines.end(),
		[] (const Line& a, const Line& b) { return a.mid.x < b.mid.x; });
	for (size_t i = 0; i < intersections_lines.size(); i++)
		if (intersections_lines[i].dir == 2) {
			if (intersections_lines[i].mid == cnt_v.mid)
				cnt_v_indx = index;
			intersections_lines[i].index = index++;
		}

	//	назначаем индексы горизонтальным линиям
	index = 0;
	std::sort(intersections_lines.begin(), intersections_lines.end(),
			[] (const Line& a, const Line& b) { return a.mid.y < b.mid.y; });
	for (size_t i = 0; i < intersections_lines.size(); i++)
		if (intersections_lines[i].dir == 1) {
			if (intersections_lines[i].mid == cnt_h.mid)
				cnt_h_indx = index;
			intersections_lines[i].index = index++;
		}

	//	пересчитываем индексы для отсчета от центра
	for (size_t i = 0; i < intersections_lines.size(); i++)
		if (intersections_lines[i].dir == 1)
			intersections_lines[i].index = -1 * (intersections_lines[i].index - cnt_h_indx);
		else
			intersections_lines[i].index -= cnt_v_indx;

	//	определяем точки пересечения линий
	intersections.clear();
	//
	for (size_t i = 0; i < intersections_lines.size(); i++)
	{
		Line line1 = intersections_lines[i];
		line(img, line1.pt1, line1.pt2, CLR_RED, 1, LINE_AA);
		//
		for (size_t j = i + 1; j < intersections_lines.size(); j++)
		{
			Line line2 = intersections_lines[j];
			if (line1.dir == line2.dir) continue;
			cv::Point2f intersection = getIntersection(line1.pt1, line1.pt2, line2.pt1, line2.pt2);
			if (intersection.x < 0) continue;
			//
			CalibPoint new_cp;
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

int get_nearest_intersection_index(CalibPoint &pt)
{
	int idx = -1;
	double min = 10000, buf;
	//
	for (size_t i = 0; i < intersections.size(); i++)
	{
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

void find_point_mm(CalibPoint &pt)
{
	if (!intersections.size()) return;
	//
	nearest_intersection_idx = get_nearest_intersection_index(pt);
	if (nearest_intersection_idx < 0) return;
	//
	CalibPoint base_ipt = intersections[nearest_intersection_idx];
	nearest_intersection = base_ipt.point_cnt;
	//
	std::vector<CalibPoint> ipt_row = get_calib_point_line_by_index(intersections_rows, base_ipt.row).points;
	std::vector<CalibPoint> ipt_col = get_calib_point_line_by_index(intersections_cols, base_ipt.col).points;
	//
	int dir;
	//
	//	соседняя точка в строке
	int row_idx = -1;
	dir = (pt.point_cnt.x < base_ipt.point_cnt.x) ?	-1 : 1;
	for (size_t i = 0; i < ipt_row.size(); i++) {
		if (ipt_row[i].point_cnt != base_ipt.point_cnt) continue;
		//
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
	dir = (pt.point_cnt.y < base_ipt.point_cnt.y) ?	-1 : 1;
	for (size_t i = 0; i < ipt_col.size(); i++) {
		if (ipt_col[i].point_cnt != base_ipt.point_cnt) continue;

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
		(nearest_intersection_idx_col > -1))
	{
		CalibPoint row_ipt = intersections[nearest_intersection_idx_row];
		double row_k = getDistance(base_ipt.point_mm, row_ipt.point_mm) / getDistance(base_ipt.point_cnt, row_ipt.point_cnt);
		CalibPoint col_ipt = intersections[nearest_intersection_idx_col];
		double col_k = getDistance(base_ipt.point_mm, col_ipt.point_mm) / getDistance(base_ipt.point_cnt, col_ipt.point_cnt);
		//
		int dir_x = (base_ipt.point_cnt.x > pt.point_cnt.x) ? -1 : 1;
		pt.point_mm.x = base_ipt.point_mm.x + dir_x * abs(base_ipt.point_cnt.x - pt.point_cnt.x) * row_k;
		//
		int dir_y = (base_ipt.point_cnt.y > pt.point_cnt.y) ? -1 : 1;
		pt.point_mm.y = base_ipt.point_mm.y + dir_y * abs(base_ipt.point_cnt.y - pt.point_cnt.y) * col_k;
	}
}

void onMouse(int event, int x, int y, int flags, void* userdata)
{
	cv::Mat* img_0 = (cv::Mat*)userdata;
	cv::Mat img = Mat::zeros(Size(img_0->cols, img_0->rows - get_status_bar_height()), img_0->type());
	cv::Point2f pt = cv::Point2f(x, y);
	cv::Point2f pt_cnt = get_point_cnt(img, pt);

	if (event == cv::EVENT_LBUTTONDOWN)
    {
        std::cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << std::endl;
        //
        if (current_modes.count(MODE_SELECT_LINE))
		{
        	selected_idx = select_calib_line(x, y);
		}
        //
        else if (current_modes.count(MODE_SELECT_POINT))
        {
        	selected_idx = select_calib_point(x, y);
        }
        //
        else if (current_modes.count(MODE_RULER))
        {
        	if (rule_points.size() == 2) rule_points.clear();
        	//
        	rule_points.push_back({
        		pt,
				pt_cnt,
				cv::Point(0, 0),
				0, 0
        	});
        	//
        	find_point_mm(rule_points[rule_points.size() - 1]);
        }
        //
        else if (current_modes.count(MODE_ADD_USER_LINE))
        {
        	if (!new_line_points.size())
        	{
        		new_line_points.push_back(pt);
        		new_line_points.push_back(pt);
        	}
        	//
        	else if (new_line_points.size() == 2)
        	{
        		double angle = abs(getAngle(new_line_points[0], new_line_points[1]));
        		if ((angle != 0) && (angle < 50)) return;
        		//
        		Line new_line;
        		new_line.pt1 = new_line_points[0];
        		new_line.pt2 = new_line_points[1];
        		new_line.mid = getMiddle(new_line.pt1, new_line.pt2);
        		new_line.dir = (angle == 0) ? 1 : 2;
        		new_line.type = 2;
        		intersections_lines.push_back(new_line);
        		fill_intersection_points(img);
        		//
				new_line_points.clear();
        	}
        }
        //
        else if (current_modes.count(MODE_ADD_USER_POINT))
        {
        	intersections.push_back({
        		pt,
				pt_cnt,
				cv::Point(0, 0),
				0, 0
        	});
        	recount_center_points(img);
        }
    }
    else if (event == cv::EVENT_RBUTTONDOWN)
    {
        //	std::cout << "Right button of the mouse is clicked - position (" << x << ", " << y << ")" << std::endl;
    }
    else if (event == cv::EVENT_MBUTTONDOWN)
    {
    	//	std::cout << "Middle button of the mouse is clicked - position (" << x << ", " << y << ")" << std::endl;
    	if (current_modes.count(MODE_ADD_USER_LINE))
			new_line_points.clear();
    	if (current_modes.count(MODE_RULER))
    		rule_points.clear();
    	if (current_modes.count(MODE_ADD_USER_POINT))
    	{
    		intersections.erase(intersections.end() - 1);
    		recount_center_points(img);
    	}
    }
    else if (event == cv::EVENT_MOUSEMOVE)
    {
    	//	std::cout << "Mouse move over the window - position (" << x << ", " << y << ")" << std::endl;
    	if (current_modes.count(MODE_ADD_USER_LINE))
    	{
    		if (new_line_points.size() == 2)
    		{
    			new_line_points[1].x = x;
    			new_line_points[1].y = y;
    		}
    	}
    }
}

void handle_keys(cv::Mat& img)
{
	if (!current_modes.count(MODE_SELECT_LINE) &&
		!current_modes.count(MODE_SELECT_POINT))
	{
		selected_idx = -1;
	}
	//
	//	выбор режима
	if (is_key_on(VK_KEY_F))
	{
		toggle_key(VK_KEY_F);
		//
		int maxKey = modes_list.rbegin()->first;
		int mode_id = current_modes.size() ? *current_modes.begin() : 0;
		mode_id = (mode_id == maxKey) ? 0 : (mode_id + 1);
		current_modes.clear();
		current_modes.insert(mode_id);
	}
	//
	//	сформировать точки пересечения
	if (is_key_on(VK_KEY_I))
	{
		toggle_key(VK_KEY_I);
		fill_opencv_intersections_lines(img);
		fill_intersection_points(img);
		write_log("intersection points filled!");
	}
	//	сохранить точки пересечения
	if (is_key_on(VK_KEY_B))
	{
		toggle_key(VK_KEY_B);
		save_intersection_points();
		//save_intersection_lines();
	}
	//	сохранить точки пересечения в формате csv
	if (is_key_on(VK_KEY_C))
	{
		toggle_key(VK_KEY_C);
		save_intersection_points_csv();
	}
	//	загрузить точки пересечения
	if (is_key_on(VK_KEY_N))
	{
		toggle_key(VK_KEY_N);
		load_intersection_points();
		//load_intersection_lines();
	}
	//	удалить пользовательскую линию/точку
	if (is_key_on(VK_KEY_DEL))
	{
		toggle_key(VK_KEY_DEL);
		if (selected_idx == -1) return;
		//
		if (current_modes.count(MODE_SELECT_POINT))
		{
			write_log("intersections " + to_string(selected_idx) +  " delete");
			intersections.erase(intersections.begin() + selected_idx);
			fill_sorted_cols_rows();
			recount_center_points(img);
		}
		//
		if (current_modes.count(MODE_SELECT_LINE))
		{
			write_log("intersection_line " + to_string(selected_idx) +  " delete");
			intersections_lines.erase(intersections_lines.begin() + selected_idx);
			fill_intersection_points(img);
		}
		//
		selected_idx = -1;
	}
	//	сместить пользовательскую линию/точку вверх
	if (is_key_on(VK_KEY_UP))
	{
		toggle_key(VK_KEY_UP);
		if (selected_idx == -1) return;
		//
		if (current_modes.count(MODE_SELECT_POINT))
		{
			write_log("intersections " + to_string(selected_idx) +  " up");
			intersections[selected_idx].point.y -= 1;
			fill_sorted_cols_rows();
			recount_center_points(img);
		}
		//
		if (current_modes.count(MODE_SELECT_LINE))
		{
			write_log("intersection_line " + to_string(selected_idx) +  " up");
			intersections_lines[selected_idx].pt1.y -= 1;
			intersections_lines[selected_idx].pt2.y -= 1;
			intersections_lines[selected_idx].mid.y -= 1;
			fill_intersection_points(img);
		}
	}
	//	сместить пользовательскую линию/точку вниз
	if (is_key_on(VK_KEY_DOWN))
	{
		toggle_key(VK_KEY_DOWN);
		if (selected_idx == -1) return;
		//
		if (current_modes.count(MODE_SELECT_POINT))
		{
			write_log("intersections " + to_string(selected_idx) +  " down");
			intersections[selected_idx].point.y += 1;
			fill_sorted_cols_rows();
			recount_center_points(img);
		}
		//
		if (current_modes.count(MODE_SELECT_LINE))
		{
			write_log("intersection_line " + to_string(selected_idx) +  " down");
			intersections_lines[selected_idx].pt1.y += 1;
			intersections_lines[selected_idx].pt2.y += 1;
			intersections_lines[selected_idx].mid.y += 1;
			fill_intersection_points(img);
		}
	}
	//	сместить пользовательскую линию/точку влево
	if (is_key_on(VK_KEY_LEFT))
	{
		toggle_key(VK_KEY_LEFT);
		if (selected_idx == -1) return;
		//
		if (current_modes.count(MODE_SELECT_POINT))
		{
			write_log("intersections " + to_string(selected_idx) +  " left");
			intersections[selected_idx].point.x -= 1;
			fill_sorted_cols_rows();
			recount_center_points(img);
		}
		//
		if (current_modes.count(MODE_SELECT_LINE))
		{
			write_log("intersection_line " + to_string(selected_idx) +  " left");
			intersections_lines[selected_idx].pt1.x -= 1;
			intersections_lines[selected_idx].pt2.x -= 1;
			intersections_lines[selected_idx].mid.x -= 1;
			fill_intersection_points(img);
		}
	}
	//	сместить пользовательскую линию/точку вправо
	if (is_key_on(VK_KEY_RIGHT))
	{
		toggle_key(VK_KEY_RIGHT);
		if (selected_idx == -1) return;
		//
		if (current_modes.count(MODE_SELECT_POINT))
		{
			write_log("intersections " + to_string(selected_idx) +  " right");
			intersections[selected_idx].point.x += 1;
			fill_sorted_cols_rows();
			recount_center_points(img);
		}
		//
		if (current_modes.count(MODE_SELECT_LINE))
		{
			write_log("intersection_line " + to_string(selected_idx) +  " left");
			intersections_lines[selected_idx].pt1.x += 1;
			intersections_lines[selected_idx].pt2.x += 1;
			intersections_lines[selected_idx].mid.x += 1;
			fill_intersection_points(img);
		}
	}
}

void draw_new_line(cv::Mat& img)
{
	if  (current_modes.count(MODE_ADD_USER_LINE) && (new_line_points.size() == 2))
	{
		cv::line(img, new_line_points[0], new_line_points[1], CLR_GREEN, 1, cv::LINE_AA, 0);
	}
}

void draw_intersection_lines(cv::Mat& img)
{
	for (size_t i = 0; i < intersections_lines.size(); i++)
		cv::line(img,
			intersections_lines[i].pt1, intersections_lines[i].pt2,
			((i == selected_idx) && current_modes.count(MODE_SELECT_LINE)) ? CLR_GREEN : CLR_RED,
			1, cv::LINE_AA, 0
		);
}

void draw_intersection_points(cv::Mat& img)
{
	int fontFace = 1;
	double fontScale = 0.5;
	Scalar fontColor = CLR_GREEN;	//	CLR_RED;	CLR_GREEN;
	int circleRadius = CALIB_PT_R;	//	4;
	//
	bool show_cols_rows = true;
	//
	int lbl_row = 0;
	int lbl_start = -2;
	int lbl_offset = 8;
	//
	for (size_t i = 0; i < intersections_cols.size(); i++)
		for (size_t j = 0; j < intersections_cols[i].points.size(); j++)
		{
			CalibPoint cp = intersections_cols[i].points[j];
			//
//			bool fl1 = false, fl2 = false, fl3 = false;
//			if (cp.point_cnt == nearest_intersection)
//				fl1 = true;
//			if (cp.point_cnt == nearest_intersection_col)
//				fl2 = true;
//			if (cp.point_cnt == nearest_intersection_row)
//				fl3 = true;
//			Scalar clr = (fl1 || fl2 || fl3)
//				? CLR_MAGENTA
//				: CLR_RED;
			int index = get_vector_calib_point_index(intersections, cp);
			Scalar clr =
				current_modes.count(MODE_SELECT_POINT) && (index == selected_idx)
				? CLR_GREEN : CLR_RED;
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
//					cp.point + cv::Point2f(5, 12),
//					fontFace, fontScale, fontColor);
			//
//			putText(img,
//				show_cols_rows ? to_string(cp.col) : to_string((int)cp.point_cnt.x),
//				cp.point + cv::Point2f(5, lbl_start + lbl_offset * lbl_row++),
//				fontFace, fontScale, fontColor);
//			putText(img,
//				show_cols_rows ? to_string(cp.row) : to_string((int)cp.point_cnt.y),
//				cp.point + cv::Point2f(5, lbl_start + lbl_offset * lbl_row++),
//				fontFace, fontScale, fontColor);
			lbl_row = 0;
		}
}

void draw_intersection_points_2(cv::Mat& img)
{
	int fontFace = 1;
	double fontScale = 0.5;
	Scalar fontColor = CLR_GREEN;	//	CLR_RED;	CLR_GREEN;
	int circleRadius = CALIB_PT_R;	//	4;
	//
	bool show_cols_rows = true;
	//
	int lbl_row = 0;
	int lbl_start = -2;
	int lbl_offset = 8;
	//
	for (size_t i = 0; i < intersections.size(); i++)
	{
		CalibPoint cp = intersections[i];
		//
		Scalar clr =
			current_modes.count(MODE_SELECT_POINT) && (i == selected_idx)
			? CLR_GREEN : CLR_RED;
		//
		cv::circle(img, cp.point, circleRadius, clr, 1, cv::LINE_AA);
		cv::line(img,
			cp.point - cv::Point2f(0, CALIB_PT_CROSS),
			cp.point + cv::Point2f(0, CALIB_PT_CROSS),
			clr, 1, cv::LINE_AA, 0);
		cv::line(img,
			cp.point - cv::Point2f(CALIB_PT_CROSS, 0),
			cp.point + cv::Point2f(CALIB_PT_CROSS, 0),
			clr, 1, cv::LINE_AA, 0);
		//
		putText(img,
			show_cols_rows ? to_string(cp.col) : to_string((int)cp.point_cnt.x),
			cp.point + cv::Point2f(5, lbl_start + lbl_offset * lbl_row++),
			fontFace, fontScale, fontColor);
		putText(img,
			show_cols_rows ? to_string(cp.row) : to_string((int)cp.point_cnt.y),
			cp.point + cv::Point2f(5, lbl_start + lbl_offset * lbl_row++),
			fontFace, fontScale, fontColor);
		//
		lbl_row = 0;
	}
}

const int mode_start = 20;
const int mode_offset = 20;

int get_status_bar_height()
{
	int cmsz = current_modes.size();
	return mode_start + mode_offset * (cmsz ? cmsz : 1) - 15;
}

void calibration(cv::Mat& img)
{
	std::vector<cv::Mat> merged;

	cv::Point cnt(img.cols / 2, img.rows / 2);
	//
	handle_keys(img);
	//
	draw_intersection_lines(img);
	draw_intersection_points(img);
	draw_intersection_points_2(img);
	draw_ruler_points(img);
	draw_new_line(img);
	//
	cv::line(img, cv::Point2f(cnt.x, 0), cv::Point2f(cnt.x, img.rows),
		CLR_YELLOW, 1, cv::LINE_AA, 0);
	cv::line(img, cv::Point2f(0, cnt.y), cv::Point2f(img.cols, cnt.y),
		CLR_YELLOW, 1, cv::LINE_AA, 0);
	//
	merged.push_back(img);
	//
	Mat status = Mat::zeros(Size(img.cols, get_status_bar_height()), img.type());
	//
	int mode_row = 0;
	for (const auto& [mode_id, mode_name] : modes_list)
		if (mode_id && current_modes.count(mode_id))
			putText(status, mode_name,
				cv::Point(5, mode_start + mode_offset * (mode_row++)),
				1, 1.5, CLR_WHITE, 2
			);
	merged.push_back(status);
	//
	if (merged.size() > 0)
		cv::vconcat(merged, img);
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

int select_calib_line(int x, int y)
{
	for (size_t i = 0; i < intersections_lines.size(); i++)
		if (getDistanceToLine(Point2f(x, y), intersections_lines[i].pt1, intersections_lines[i].pt2) < 3)
			return i;
	return -1;
}

int select_calib_point(int x, int y)
{
	CalibPoint cp;
	for (size_t i = 0; i < intersections.size(); i++)
	{
		cp = intersections[i];
		int x1 = cp.point.x - CALIB_PT_R;
		int x2 = cp.point.x + CALIB_PT_R;
		int y1 = cp.point.y - CALIB_PT_R;
		int y2 = cp.point.y + CALIB_PT_R;
		//
		if ((x >= x1) && ( x <= x2) && (y >= y1) && (y <= y2))
		{
			if (i != selected_idx) return i;
				else return -1;
		}
	}
	return -1;
}

void draw_ruler_points(cv::Mat& img)
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
		CalibPoint pt = rule_points[i];
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
