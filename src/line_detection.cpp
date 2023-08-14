#include "opencv2/opencv.hpp"

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time.h>

#include <math.h>

#include <iomanip>
#include <cstdlib>
#include <libconfig.h++>

using namespace libconfig;

#include <arpa/inet.h>
#include <sys/socket.h>

#include <thread>

char UDP_ADDR[15];	//	"192.168.1.100"
#define UDP_BUFLEN 255
int UDP_PORT = 0;	//	8888

using namespace cv;
using namespace std;

#define NUM_ROI		4	//	общее количество "строк"
#define NUM_ROI_H	4	//	количество "строк" снизу, которые делим на "столбцы"
#define NUM_ROI_V	3	//	количество "столбцов" в "строке"
#define DATA_SIZE (NUM_ROI_H * NUM_ROI_V + (NUM_ROI - NUM_ROI_H))

#define HOR_ANGLE		10
#define VER_ANGLE		5
#define MIN_CONT_LEN	(100.0)
#define HOR_COLLAPSE	100

#define DIR_LEFT	-1
#define DIR_RIGHT	 1
#define DIR_FORWARD	 0

#define CLR_BLACK		(cv::Scalar(0x00, 0x00, 0x00))
#define CLR_RED			(cv::Scalar(0x00, 0x00, 0xFF))
#define CLR_BLUE		(cv::Scalar(0xFF, 0x00, 0x00))
#define CLR_GREEN		(cv::Scalar(0x00, 0xFF, 0x00))
#define CLR_YELLOW		(cv::Scalar(0x00, 0xFF, 0xFF))
#define CLR_MAGENTA		(cv::Scalar(0xFF, 0x00, 0xFF))
#define CLR_CYAN		(cv::Scalar(0xFF, 0xFF, 0x00))

#define CLR_RECT_BOUND	(cv::Scalar(0xFF, 0x33, 0x33))

string CAM_ADDR;	//	"rtsp://admin:1234qwer@192.168.1.64:554/streaming/channels/2"

#define USE_CAMERA	1
#define DETAILED	0
#define SHOW_GRAY	0
#define DRAW_GRID	0
#define DRAW		0
#define MORPHOLOGY	1

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
	std::vector<RectData> vRect;
};

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

void get_contour(cv::Mat& imgColor, cv::Mat& imgGray, cv::Rect& roi, ContData& dataItem, int roi_row, int roi_col) {

	if (DRAW && DRAW_GRID)
		cv::rectangle(imgColor, roi, CLR_YELLOW, 1, cv::LINE_AA, 0);

	get_contur_params(imgGray, roi, dataItem, MIN_CONT_LEN, roi_row, roi_col);

}

bool check_points_horz(const cv::Point& pt1, const cv::Point& pt2, const double angle) {
	/*
	double a = findPointsAngle(pt1, pt2);
	if (a < 0) a *= -1;
	a = abs(180 - a);
	return (a <= angle);
	*/
	return abs(pt1.y - pt2.y) < 20;
}

bool check_rects_adj_vert(cv::Rect r1, const cv::Rect r2) {
	//const int delta = 0;
	// Если один прямоугольник находится ниже другого
	if ((r1.y > (r2.y + r2.height)) ||
		(r2.y > (r1.y + r1.height))) {
		return false;
	}
	// В противном случае, прямоугольники пересекаются
	return true;
}

bool check_rects_adj_horz(const cv::Rect r1, const cv::Rect r2) {
	// Если один прямоугольник находится справа от другого
	if ((r1.x > (r2.x + r2.width)) ||
		(r2.x > (r1.x + r1.width))) {
		return false;
	}
	// В противном случае, прямоугольники пересекаются
	return true;
}

bool check_horz_bounds(std::vector<RectData*> line) {
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

int get_hor_line_y(cv::Mat& imgColor, std::vector<RectData*> line) {
	int y = 0;
	for (size_t j = 0; j < line.size(); j++) {
		cv::Point pt = line[j]->center;
		y += pt.y;
		//
		if (DRAW && DETAILED)
			cv::circle(imgColor, pt, 3, CLR_RED, 1, cv::LINE_AA);
	}
	y /= line.size();
	return y;
}

void parse_image(cv::Mat imgColor, std::vector<cv::Point>& res_points, std::vector<int>& hor_ys, bool& fl_error)
{
	cv::Mat trImage(imgColor.rows, imgColor.cols, CV_8U);
	cv::cvtColor(imgColor, trImage, cv::COLOR_BGR2GRAY);

	cv::GaussianBlur(trImage, trImage, Size(9, 9), 0);

	if (MORPHOLOGY) {
		cv::morphologyEx(trImage, trImage, MORPH_OPEN, getStructuringElement(MORPH_RECT, Size(7, 7)));
		//cv::morphologyEx(trImage, trImage, MORPH_CLOSE, getStructuringElement(MORPH_RECT, Size(7, 7)));
	}

	cv::Mat gray; //(imgColor.rows, imgColor.cols, CV_8U);
	cv::threshold(trImage, gray, 160, 255, cv::THRESH_BINARY_INV);

	ContData data[(DATA_SIZE)];
//	char buff[30];

	int imgWidth = imgColor.cols;
	int imgHeight = imgColor.rows;
	int imgOffset = imgHeight / NUM_ROI;
	int imgOffsetV = imgWidth / NUM_ROI_V;
//	int centerX = imgWidth / 2;
//	int centerY = imgHeight / 2;

	fl_error = false;

	int k = 0;

	for (int i = 0; i < NUM_ROI; ++i)
	{
		if (i < (NUM_ROI - NUM_ROI_H)) {
			cv::Rect roi(0, imgOffset * i, imgWidth, imgOffset);
			get_contour(imgColor, gray, roi, data[k], i, 0);
			k++;
		} else {
			for (int j = 0; j < NUM_ROI_V; ++j)
			{
				cv::Rect roi(imgOffsetV * j, imgOffset * i, imgOffsetV, imgOffset);
				get_contour(imgColor, gray, roi, data[k], i, j);
				k++;
			}
		}
	}

	cv::Point pt(imgWidth / 2, imgHeight);
	cv::Point* lpCenter = &pt;

	res_points.clear();

	std::vector<RectData*> buf_points;
	//
	std::vector<RectData*> hor_points;
	std::vector<std::vector<RectData*>> hor_lines;
	hor_ys.clear();

	for (int i = 0; i < (DATA_SIZE); ++i)
	{
		ContData* dt = &data[(DATA_SIZE) - 1 - i];
		RectData* rd = sort_cont(*lpCenter, dt[0]);

		if (rd == nullptr)
			continue;

		buf_points.push_back(rd);

		lpCenter = &rd->center;

		if (DRAW && DETAILED) {
			cv::rectangle(imgColor, rd->bound, CLR_RECT_BOUND);
			cv::circle(imgColor, rd->center, 3, CLR_GREEN, 1, cv::LINE_AA);
		}
	}

	std::sort(buf_points.begin(), buf_points.end(),
		[](RectData* a, RectData* b) { return (a->bound.y > b->bound.y); });

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
				if (check_points_horz(rd1->center, rd2->center, HOR_ANGLE)) {
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
				if (abs(y1 - y2) < HOR_COLLAPSE) {
					y_sum += y2;
					y_cnt++;
					k = j;
				}
			}
			hor_ys.push_back(y_sum / y_cnt);
			i = (k > 0) ? (k + 1) : (i + 1);
		}
	}

	//	добавляем нижнюю центральную точку в список для построения линии
	RectData crd;
	crd.center = cv::Point(imgWidth / 2, imgHeight);
	crd.bound = cv::Rect(crd.center.x - imgOffsetV / 2, crd.center.y - 10, imgOffsetV, 20);
	crd.roi_row = NUM_ROI;
	buf_points.insert(buf_points.begin(), &crd);

	//	строим линию
	{
		size_t i = 0;
		RectData* rd1;
		RectData* rd2;
		while (i < buf_points.size()) {
			int k = 0;
			double min_len = 100000.0;
			rd1 = buf_points[i];
			for (size_t j = i + 1; j < buf_points.size(); j++) {
				rd2 = buf_points[j];
				if (abs(rd1->roi_row - rd2->roi_row) != 1)
					continue;
				//
				if (
					true
					//&& ((rd1->roi_row - rd2->roi_row) == 1)				//	если точки в соседних строках
					&& check_rects_adj_vert(rd1->bound, rd2->bound)		//	если области пересекаются по вертикали
					&& check_rects_adj_horz(rd1->bound, rd2->bound)		//	если области пересекаются по горизонтали
					)
				{
					double len = length(rd1->center, rd2->center);
					if (len <= min_len) {
						min_len = len;
						k = j;
					}
				}
			}
			if (k > 0)
				res_points.push_back(buf_points[k]->center);
			else break;	//	если не можем построить следующий отрезок, то прекращаем обработку (сигнализировать об ошибке?)
			i = (k > 0) ? k : (i + 1);
		}
	}

	fl_error = fl_error || (res_points.size() < NUM_ROI);

	if (DRAW) {
		if (fl_error) {
			cv::circle(imgColor, cv::Point(50, 50), 20, CLR_RED, -1, cv::LINE_AA);
		}
		else {
			for (size_t i = 0; i < hor_ys.size(); i++)
				cv::line(imgColor, cv::Point(0, hor_ys[i]), cv::Point(imgWidth, hor_ys[i]),
					CLR_RED, 2, cv::LINE_AA, 0);
			//
			cv::Point line_pt(imgWidth / 2, imgHeight);
			for (size_t i = 0; i < res_points.size(); i++) {
				cv::line(imgColor, line_pt, res_points[i], CLR_GREEN, 2, cv::LINE_AA, 0);
				line_pt = res_points[i];
			}
		}
		//
		cv::imshow("img", imgColor);
		if (SHOW_GRAY) cv::imshow("gray", gray);
		//cv::imshow("thresh", trImage);
	}

}

void read_config(char* exe) {

	Config cfg;

	char cfg_filename[255];

	sprintf(cfg_filename, "%s.cfg", exe);

	try
	{
		cfg.readFile(cfg_filename);
	}
	catch(const FileIOException &fioex)
	{
		std::cerr << "I/O error while reading file." << std::endl;
		exit(1);
	}
	catch(const ParseException &pex)
	{
		std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
				  << " - " << pex.getError() << std::endl;
		exit(1);
	}

	try {

		//NUM_ROI = cfg.lookup("roi_amount");

		string cam_addr_1 = cfg.lookup("camera_address_1");
		CAM_ADDR = cam_addr_1;

		string udp_addr_str = cfg.lookup("udp_address");
		strcpy(UDP_ADDR, udp_addr_str.c_str());

		UDP_PORT = cfg.lookup("udp_port");

	}
	catch(const SettingNotFoundException &nfex)
	{
		cerr << "Critical setting was not found in configuration file." << endl;
		exit(1);
	}

}

struct udp_point {
	uint16_t x;
	uint16_t y;
};

struct udp_package {
	uint16_t counter;
	uint16_t img_width;
	uint16_t img_height;
	uint8_t error_flag;
	uint8_t points_count;
	udp_point points[4];
	uint16_t points_hor[4];
};

#define AVG_CNT	1
#define STATS_LOG 0
#define UDP_LOG 0

int main(int argc, char** argv)
{

	//setenv("OPENCV_FFMPEG_CAPTURE_OPTIONS", "rtsp_transport;udp", 1);
	//-rtsp_transport tcp -protocol_whiteliste rtp,file,udp,tcp

	unsigned int n = std::thread::hardware_concurrency();
    std::cout << n << " concurrent threads are supported.\n";

	read_config(argv[0]);

	clock_t tStart;

	//const int avg_cnt = 20;
	double tt, sum = 0;
	int cnt = 0;
	int incorrect_lines = 0;

	uint64_t read_err = 0;

	std::vector<cv::Point> res_points;
	std::vector<int> hor_ys;

	struct sockaddr_in si_other;
	int s, slen = sizeof(si_other);

	if ( (s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 )
	{
		fprintf(stderr, "socket");
		exit(1);
	}

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(UDP_PORT);

	if (inet_aton(UDP_ADDR, &si_other.sin_addr) == 0)
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	cv::VideoCapture cap(CAM_ADDR);
	//cap.set(38, 3); // CV_CAP_PROP_BUFFERSIZE = 38

	if (!cap.isOpened()) {
		std::cerr << "Error opening camera." << std::endl;
		return -1;
	}

	cv::Mat frame;

	// Очищаем буфер
	for (int i = 0; i < 20; i++)
		cap >> frame;

	uint16_t counter = 0;

	udp_package udp_pack;

	bool fl_error;

	while (true) {

		tStart = clock();

		try {
			//read_total++;
			if (!(cap.read(frame)))
				read_err++;
			//cap >> frame;
		} catch (...) {
			printf("read error!\n");
		}

		try {
			parse_image(frame, res_points, hor_ys, fl_error);
		} catch (...) {
			printf("parse error!\n");
		}

		if (fl_error)
			incorrect_lines++;

		tt = (double)(clock() - tStart) / CLOCKS_PER_SEC;
		sum += tt;
		cnt++;

		if (cnt >= AVG_CNT)
		{

			memset(&udp_pack, 0, sizeof(udp_pack));

			counter++;

			udp_pack.counter = counter;
			udp_pack.img_width = frame.cols;
			udp_pack.img_height = frame.rows;
			udp_pack.error_flag = (fl_error) ? 1 : 0;
			//
			udp_pack.points_count = hor_ys.size() & 0xF;
			udp_pack.points_count |= (res_points.size() & 0xF) << 4;
			//
			for (size_t i = 0; i < res_points.size(); i++)
			{
				if (UDP_LOG) printf("(%d; %d) ", res_points[i].x, res_points[i].y);
				udp_pack.points[i] = { (uint16_t)res_points[i].x, (uint16_t)res_points[i].y };
			}
			if (UDP_LOG) printf("\n");
			//
			for (size_t i = 0; i < hor_ys.size(); i++)
			{
				if (UDP_LOG) printf("(%d) ", hor_ys[i]);
				udp_pack.points_hor[i] = { (uint16_t)hor_ys[i] };
			}
			if (UDP_LOG) printf("\n");
			//
			size_t sz = sizeof(udp_pack);
			if (UDP_LOG) {
				char* my_s_bytes = reinterpret_cast<char*>(&udp_pack);
				for (size_t i = 0; i < sz; i++)
					printf("%02x ", my_s_bytes[i]);
				printf("\n");
			}
			//
			if ( sendto(s, &udp_pack, sz, 0, (struct sockaddr *) &si_other, slen) == -1 )
			{
				fprintf(stderr, "sendto()");
				exit(1);
			}
			//
			if (STATS_LOG) {
				if (read_err)
					printf("Reading errors = %lu\n", read_err);
				printf("Incorrect lines: %d\n", incorrect_lines);
				printf("Average time taken: %.3fs\n", sum / cnt);
			}
			//
			incorrect_lines = 0;
			cnt = 0;
			sum = 0;
		}

		if (cv::waitKey(1) == 27)
			break;

	}

	//close(s);

    return 0;
}
