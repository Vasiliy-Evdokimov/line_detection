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
#include <thread>
#include <pthread.h>

using namespace cv;
using namespace std;

#include "defines.hpp"
#include "config.hpp"
#include "contours.hpp"
#include "horizontal.hpp"
#include "barcode.hpp"
#include "shared_memory.hpp"
#include "camera.hpp"

mutex frames_mtx[2];
cv::Mat frames_to_show[2];
cv::Mat grays_to_show[2];

mutex parse_results_mtx[2];
ResultFixed parse_results[2];

void visualizer_func()
{

	if (THREAD_NAMING)
		pthread_setname_np(pthread_self(), "visualizer thread");

	cv::Mat mergedGray;
	cv::Mat mergedColor;

	cout << "visualizer_func() started!\n";
	cout << "visualizer_func() entered infinity loop.\n";

	cv::Mat frames[2];
	cv::Mat grays[2];

	while (!kill_threads) {

		for	(int i = 0; i < 2; i++) {
			frames_mtx[i].lock();
			if (!(frames_to_show[i].empty()))
				frames[i] = frames_to_show[i].clone();
			if (config.SHOW_GRAY && !(grays_to_show[i].empty()))
				grays[i] = grays_to_show[i].clone();
			frames_mtx[i].unlock();
		}

		if (config.SHOW_GRAY && !(grays[0].empty() || grays[1].empty()))
			cv::hconcat(grays[0], grays[1], mergedGray);
		if (!(frames[0].empty() || frames[1].empty()))
			cv::hconcat(frames[0], frames[1], mergedColor);

		//
		if (config.SHOW_GRAY && !mergedGray.empty())
			cv::imshow("Cameras 1 & 2 Gray", mergedGray);
		if (!mergedColor.empty())
			cv::imshow("Cameras 1 & 2 Color", mergedColor);
		//
		cv::waitKey(1);

	}

	destroyAllWindows();

	cout << "visualizer_func() is out of infinity loop.\n";

}

void camera_func(string aThreadName, string aCamAddress, int aIndex)
{

	pthread_setname_np(pthread_self(), aThreadName.c_str());

    cout << aThreadName <<  " started!\n";

	clock_t tStart;

	double tt, sum = 0;
	int cnt = 0;
	int incorrect_lines = 0;

	uint64_t read_err = 0;

	cv::VideoCapture cap(aCamAddress);

	if (!cap.isOpened()) {
		cerr <<  aThreadName << " - error opening camera!\n";
		return;
	}

	cv::Mat frame;

	//	очищаем буфер
	for (int i = 0; i < CLEAR_CAM_BUFFER; i++)
		cap >> frame;

	cout << aThreadName <<  " entered infinity loop.\n";

	ParseImageResult parse_result;
	ResultFixed rfx;

	while (1) {

		if (restart_threads || kill_threads) break;

		tStart = clock();

		try {
			if (!(cap.read(frame)))
				read_err++;
		}
		catch (const std::exception& e) {
			cout << aThreadName << " an exception occurred: " << e.what() << endl;
		}
		catch (...) {
			cout << aThreadName << " read error!\n";
		}

		parse_result.width = frame.cols;
		parse_result.height = frame.rows;

		try {

			//	ищем центры областей и горизонтальные пересечения
			parse_image(
				aThreadName,
				frame,
				parse_result.res_points,
				parse_result.hor_ys,
				parse_result.fl_error,
				aIndex
			);
			//
			//parse_results_mtx[aIndex].lock();
			//parse_results[aIndex] = parse_result.ToFixed();
			//write_results_sm(parse_results[aIndex], aIndex);
			rfx = parse_result.ToFixed();
			write_results_sm(rfx, aIndex);
			//parse_results_mtx[aIndex].unlock();

		} catch (...) {
			cout << aThreadName <<  " parse error!\n";
		}

		if (STATS_LOG) {

			if (parse_result.fl_error)
				incorrect_lines++;
			//
			tt = (double)(clock() - tStart) / CLOCKS_PER_SEC;
			sum += tt;
			cnt++;
			//
			if (cnt >= AVG_CNT)
			{
				if (read_err)
					cout << aThreadName <<  "thread Reading errors = " << read_err << endl;
				cout << aThreadName <<  "thread Incorrect lines: " << incorrect_lines << endl;
				cout << aThreadName <<  "thread Average time taken: " << sum / cnt << endl;
				//
				incorrect_lines = 0;
				cnt = 0;
				sum = 0;
			}

		}
	}

	cap.release();

	cout << aThreadName <<  " is out of infinity loop.\n";

}

void parse_image(string aThreadName, cv::Mat imgColor,
	vector<cv::Point>& res_points, vector<int>& hor_ys,
	bool& fl_error, int aIndex)
{

	cv::Mat trImage(imgColor.rows, imgColor.cols, CV_8U);
	cv::cvtColor(imgColor, trImage, cv::COLOR_BGR2GRAY);

	Size2i gbk = Size(config.GAUSSIAN_BLUR_KERNEL, config.GAUSSIAN_BLUR_KERNEL);
	cv::GaussianBlur(trImage, trImage, gbk, 0);

	Size2i mok = Size(config.MORPH_OPEN_KERNEL, config.MORPH_OPEN_KERNEL);
	cv::morphologyEx(trImage, trImage, MORPH_OPEN, getStructuringElement(MORPH_RECT, mok));
	Size2i mck = Size(config.MORPH_CLOSE_KERNEL, config.MORPH_CLOSE_KERNEL);
	cv::morphologyEx(trImage, trImage, MORPH_CLOSE, getStructuringElement(MORPH_RECT, mck));

	cv::Mat gray;
	cv::threshold(trImage, gray,
		config.THRESHOLD_THRESH, config.THRESHOLD_MAXVAL, cv::THRESH_BINARY_INV);

#ifndef NO_GUI
	if (config.DRAW && config.SHOW_GRAY) {
		frames_mtx[aIndex].lock();
		grays_to_show[aIndex] = gray.clone();
		frames_mtx[aIndex].unlock();
	}
#endif

	ContData data[config.DATA_SIZE];

	int imgWidth = imgColor.cols;
	int imgHeight = imgColor.rows;
	int imgOffset = imgHeight / config.NUM_ROI;
	int imgOffsetV = imgWidth / config.NUM_ROI_V;

	fl_error = false;

	int k = 0;

	for (int i = 0; i < config.NUM_ROI; ++i)
	{
		if (i < (config.NUM_ROI - config.NUM_ROI_H)) {
			cv::Rect roi(0, imgOffset * i, imgWidth, imgOffset);
			get_contour(imgColor, gray, roi, data[k], i, 0);
			k++;
		} else {
			for (int j = 0; j < config.NUM_ROI_V; ++j)
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

	for (int i = 0; i < config.DATA_SIZE; ++i)
	{
		ContData* dt = &data[config.DATA_SIZE - 1 - i];
		RectData* rd = sort_cont(*lpCenter, dt[0]);

		if (rd == nullptr)
			continue;

		buf_points.push_back(rd);

		lpCenter = &rd->center;

#ifndef NO_GUI
		if (config.DRAW && config.DRAW_DETAILED) {
			cv::rectangle(imgColor, rd->bound, CLR_RECT_BOUND);
			cv::circle(imgColor, rd->center, 3, CLR_GREEN, 1, cv::LINE_AA);
		}
#endif
	}

	sort(buf_points.begin(), buf_points.end(),
		[](RectData* a, RectData* b) { return (a->bound.y > b->bound.y); });

	//	поиск горизонтальных пересечений
	find_horizontal(imgColor, buf_points, hor_ys);

	//	добавляем нижнюю центральную точку в список для построения линии
	RectData crd;
	crd.center = cv::Point(imgWidth / 2, imgHeight);
	crd.bound = cv::Rect(crd.center.x - imgOffsetV / 2, crd.center.y - 10, imgOffsetV, 20);
	crd.roi_row = config.NUM_ROI;
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

	fl_error = fl_error || (res_points.size() < config.NUM_ROI);

	//	поиск и рисование штрихкодов
	find_barcodes(imgColor);

#ifndef NO_GUI
	if (config.DRAW) {
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
		frames_mtx[aIndex].lock();
		frames_to_show[aIndex] = imgColor.clone();
		frames_mtx[aIndex].unlock();
	}
#endif

}
