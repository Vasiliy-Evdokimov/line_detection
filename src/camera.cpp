/*
 * camera.cpp
 *
 *  Created on: Aug 17, 2023
 *      Author: vevdokimov
 */

#include "opencv2/opencv.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <string.h>

using namespace cv;
using namespace std;

#include "defines.hpp"
#include "contours.hpp"
#include "horizontal.hpp"
#include "udp.hpp"
#include "camera.hpp"

void camera_func(string aThreadName, string aCamAddress, int aIndex)
{

    cout << aThreadName <<  "thread started!\n";

	vector<cv::Point> res_points;
	vector<int> hor_ys;

	clock_t tStart;

	double tt, sum = 0;
	int cnt = 0;
	int incorrect_lines = 0;

	uint64_t read_err = 0;

	cv::VideoCapture cap(aCamAddress);

	if (!cap.isOpened()) {
		cerr << "Error opening camera." << endl;
		return;
	}

	cv::Mat frame;

	// Очищаем буфер
	for (int i = 0; i < 20; i++)
		cap >> frame;

	uint16_t counter = 0;

	udp_package udp_pack;

	bool fl_error;

	cout << aThreadName <<  "thread entered infinity loop.\n";

	while (true) {

		tStart = clock();

		try {
			//read_total++;
			if (!(cap.read(frame)))
				read_err++;
			//cap >> frame;
		} catch (...) {
			cout << aThreadName <<  "thread read error!\n";
		}

		try {
			parse_image(aThreadName, frame, res_points, hor_ys, fl_error);
		} catch (...) {
			cout << aThreadName <<  "thread parse error!\n";
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
			udp_packs_mtx.lock();
			memcpy(&udp_packs[aIndex], &udp_pack, sizeof(udp_pack));
			udp_packs_mtx.unlock();
			//
			size_t sz = sizeof(udp_pack);
			if (UDP_LOG) {
				char* my_s_bytes = reinterpret_cast<char*>(&udp_pack);
				for (size_t i = 0; i < sz; i++)
					printf("%02x ", my_s_bytes[i]);
				printf("\n");
			}
			//
			if (STATS_LOG) {
				if (read_err)
					cout << aThreadName <<  "thread Reading errors = " << read_err << endl;
				cout << aThreadName <<  "thread Incorrect lines: " << incorrect_lines << endl;
				cout << aThreadName <<  "thread Average time taken: " << sum / cnt << endl;
			}
			//
			incorrect_lines = 0;
			cnt = 0;
			sum = 0;
		}

		if (cv::waitKey(1) == 27)
			break;

	}

}

void parse_image(string aThreadName, cv::Mat imgColor,
	vector<cv::Point>& res_points, vector<int>& hor_ys,
	bool& fl_error)
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

	vector<RectData*> buf_points;

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

	sort(buf_points.begin(), buf_points.end(),
		[](RectData* a, RectData* b) { return (a->bound.y > b->bound.y); });

	//	searching for horizontal intersections
	find_horizontal(imgColor, buf_points, hor_ys);

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
		cv::imshow(aThreadName + "img", imgColor);
		if (SHOW_GRAY) cv::imshow(aThreadName + "gray", gray);
		//cv::imshow(aThreadName + "thresh", trImage);
	}

}
