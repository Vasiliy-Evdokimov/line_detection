/*
 * camera.cpp
 *
 *  Created on: Aug 17, 2023
 *      Author: vevdokimov
 */

#include "defines.hpp"
#include "common_types.hpp"
#include "log.hpp"

#include "opencv2/opencv.hpp"
#include "opencv2/objdetect/objdetect.hpp"

#ifndef NO_GUI
	#include "opencv2/highgui/highgui.hpp"
#endif

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"

#include <string.h>
#include <thread>
#include <pthread.h>

using namespace cv;
using namespace std;

#include "config.hpp"
#include "contours.hpp"
#include "horizontal.hpp"
#include "barcode.hpp"
#include "shared_memory.hpp"
#include "calibration.hpp"
#include "camera.hpp"

//mutex frames_mtx[CAM_COUNT];
cv::Mat sources_to_show[CAM_COUNT];
cv::Mat undistorteds_to_show[CAM_COUNT];
cv::Mat frames_to_show[CAM_COUNT];
cv::Mat grays_to_show[CAM_COUNT];

//mutex parse_results_mtx[CAM_COUNT];
ResultFixed parse_results[CAM_COUNT];

const string SOURCE_WND_NAME = "Camera(s) Source";
const string UNDISTORTED_WND_NAME = "Camera(s) Undistorted";
const string GRAY_WND_NAME = "Camera(s) Gray";
const string COLOR_WND_NAME = "Camera(s) Color";

const string CALIBRATION_WND_NAME = "Calibration editor";

cv::Mat calibration_img;

void visualizer_func()
{

	pthread_setname_np(pthread_self(), "visualizer thread");

	write_log("visualizer_func() started!");
	write_log("visualizer_func() entered infinity loop.");

	if (config.CALIBRATE_CAM) {
		namedWindow(CALIBRATION_WND_NAME);
		setMouseCallback(CALIBRATION_WND_NAME, onMouse, &calibration_img);
	}

	while (!kill_threads) {

		cv::Mat mergedSource;
		cv::Mat mergedUndistorted;
		cv::Mat mergedGray;
		cv::Mat mergedFrames;
		//
		std::vector<cv::Mat> sources;
		std::vector<cv::Mat> undistorteds;
		std::vector<cv::Mat> grays;
		std::vector<cv::Mat> frames;
		//
		for	(int i = 0; i < CAM_COUNT; i++) {
			if (!(config.USE_CAM & (1 << i))) continue;
			//
			cv::Mat source;
			cv::Mat undistorted;
			cv::Mat gray;
			cv::Mat frame;
			//
			//frames_mtx[i].lock();
			if (!(sources_to_show[i].empty()))
				source = sources_to_show[i].clone();
			if (!(undistorteds_to_show[i].empty()))
				undistorted = undistorteds_to_show[i].clone();
			if (config.SHOW_GRAY && !(grays_to_show[i].empty()))
				gray = grays_to_show[i].clone();
			if (!(frames_to_show[i].empty()))
				frame = frames_to_show[i].clone();
			//frames_mtx[i].unlock();
			//
			if (config.CALIBRATE_CAM == (i + 1))
			{
				calibration_img = undistorted.clone();
				if (!(calibration_img.empty()))
					calibration(calibration_img);
			}
			//
			if (!(source.empty()))
				sources.push_back(source);
			//
			if (!(undistorted.empty()))
				undistorteds.push_back(undistorted);
			//
			if (!(gray.empty())) {
				cv::putText(gray, "Camera " + to_string(i + 1), cv::Point2f(10, 20),
					cv::FONT_HERSHEY_DUPLEX, 0.5, CLR_BLACK);
				grays.push_back(gray);
			}
			//
			if (!(frame.empty())) {
				cv::putText(frame, "Camera " + to_string(i + 1), cv::Point2f(10, 20),
					cv::FONT_HERSHEY_DUPLEX, 0.5, CLR_GREEN);
				frames.push_back(frame);
			}
		}
		//
		if (sources.size() > 0)
			cv::hconcat(sources, mergedSource);
		if (undistorteds.size() > 0)
			cv::hconcat(undistorteds, mergedUndistorted);
		if (config.SHOW_GRAY && (grays.size() > 0))
			cv::hconcat(grays, mergedGray);
		if (frames.size() > 0)
			cv::hconcat(frames, mergedFrames);
		//
		if (config.CALIBRATE_CAM && !calibration_img.empty())
			cv::imshow(CALIBRATION_WND_NAME, calibration_img);
		//
		if (!config.CALIBRATE_CAM)
		{
//			if (!mergedSource.empty())
//				cv::imshow(SOURCE_WND_NAME, mergedSource);
//			if (!mergedUndistorted.empty())
//				cv::imshow(UNDISTORTED_WND_NAME, mergedUndistorted);
			if (config.SHOW_GRAY && !mergedGray.empty())
				cv::imshow(GRAY_WND_NAME, mergedGray);
			if (!mergedFrames.empty())
				cv::imshow(COLOR_WND_NAME, mergedFrames);
		}
		//
		int key = cv::waitKey(1);
		if (key != -1)
		{
			write_log(to_string(key));
			toggle_key(key);
		}

	}

	destroyAllWindows();

	write_log("visualizer_func() is out of infinity loop.");

}

void parse_result_to_sm(ParseImageResult& parse_result, int aIndex) {
	ResultFixed rfx = parse_result.ToFixed();
	write_results_sm(rfx, aIndex);
}

void camera_func(string aThreadName, string aCamAddress, int aIndex)
{

	pthread_setname_np(pthread_self(), aThreadName.c_str());

    write_log(aThreadName + " started!");

	clock_t tStart;

	double tt, sum = 0;
	int cnt = 0;
	int incorrect_lines = 0;

	uint64_t read_err = 0;

	cv::VideoCapture cap;
	cv::Mat frame;
	cv::Mat undistorted;

	ParseImageResult parse_result;
	parse_result.fl_err_camera = true;	//	инициализация камеры

	while (1) {

		if (restart_threads || kill_threads) break;

		if (parse_result.fl_err_camera) {

			if (cap.isOpened())
				cap.release();

			parse_result.fl_err_camera = !cap.open(aCamAddress);
			parse_result_to_sm(parse_result, aIndex);

			if (parse_result.fl_err_camera)	{
				write_log(aThreadName + " - error opening camera!");
				continue;
			}

			//	очищаем буфер
			for (int i = 0; i < CLEAR_CAM_BUFFER; i++)
				cap >> frame;

			write_log(aThreadName + " entered infinity loop.");

		}

		tStart = clock();

		parse_result.fl_err_camera = (!cap.isOpened()) || (!cap.read(frame));

		if (parse_result.fl_err_camera) {
			write_log(aThreadName + " read error!");
			parse_result_to_sm(parse_result, aIndex);
			continue;
		}

		try
		{
			undistort(frame, undistorted, cameraMatrix, distCoeffs);
		}
		catch (...)
		{
			parse_result.fl_err_camera = true;
			write_log(aThreadName + " undistortion error!");
			parse_result_to_sm(parse_result, aIndex);
			continue;
		}

		//frames_mtx[aIndex].lock();
		sources_to_show[aIndex] = frame.clone();
		undistorteds_to_show[aIndex] = undistorted.clone();
		//frames_mtx[aIndex].unlock();

		parse_result.width = frame.cols;
		parse_result.height = frame.rows;

		try {

			//	ищем центры областей и горизонтальные пересечения
			parse_image(
				aThreadName,
				undistorted,	//	frame,
				parse_result,
				aIndex
			);
			parse_result.fl_err_parse = false;
			//
			//parse_results_mtx[aIndex].lock();
			//parse_results[aIndex] = parse_result.ToFixed();
			//write_results_sm(parse_results[aIndex], aIndex);
			//parse_result_to_sm(parse_result, aIndex);
			//parse_results_mtx[aIndex].unlock();

		} catch (...) {
			parse_result.fl_err_parse = true;
		}

		parse_result_to_sm(parse_result, aIndex);

		if (parse_result.fl_err_parse) {
			write_log(aThreadName + " parse error!");
			continue;
		}

		if (STATS_LOG) {

			if (parse_result.fl_err_line)
				incorrect_lines++;
			//
			tt = (double)(clock() - tStart) / CLOCKS_PER_SEC;
			sum += tt;
			cnt++;
			//
			if (cnt >= AVG_CNT)
			{
				if (read_err)
					write_log(aThreadName + "thread Reading errors = " + to_string(read_err));
				write_log(aThreadName + "thread Incorrect lines: " + to_string(incorrect_lines));
				write_log(aThreadName + "thread Average time taken: " + to_string(sum / cnt));
				//
				incorrect_lines = 0;
				cnt = 0;
				sum = 0;
			}

		}
	}

	cap.release();

	write_log(aThreadName + " is out of infinity loop.");

}

void parse_image(string aThreadName, cv::Mat imgColor,
	ParseImageResult& parse_result, int aIndex)
{

	//	поиск штрихкодов
	std::vector<std::vector<cv::Point>> bc_contours;
	//
	find_barcodes(imgColor, parse_result, bc_contours);
	//
	int minX = imgColor.cols;
	int maxX = 0;
	for (size_t i = 0; i < bc_contours.size(); i++)
		for (size_t j = 0; j < bc_contours[i].size(); j++) {
			cv::Point point = bc_contours[i][j];
			if (point.x < minX) minX = point.x;
			if (point.x > maxX) maxX = point.x;
		}

	cv::Mat trImage(imgColor.rows, imgColor.cols, CV_8U);
	cv::cvtColor(imgColor, trImage, cv::COLOR_BGR2GRAY);

	int gbk = config.GAUSSIAN_BLUR_KERNEL;
	cv::GaussianBlur(trImage, trImage, Size2i(gbk, gbk), 0);

	int mok = config.MORPH_OPEN_KERNEL;
	cv::morphologyEx(trImage, trImage, MORPH_OPEN, getStructuringElement(MORPH_RECT, Size2i(mok, mok)));
//	int mck = config.MORPH_CLOSE_KERNEL;
//	cv::morphologyEx(trImage, trImage, MORPH_CLOSE, getStructuringElement(MORPH_RECT, Size2i(mck, mck)));

	cv::Mat gray;
	cv::threshold(trImage, gray, config.THRESHOLD_THRESH, config.THRESHOLD_MAXVAL, cv::THRESH_BINARY_INV);

//	for (size_t i = 0; i < bc_contours.size(); i++)
//		cv::rectangle(gray, cv::boundingRect(bc_contours[i]), CLR_BLACK, -1);

	if (bc_contours.size() > 0) {
		if (config.BARCODE_LEFT)
			cv::rectangle(gray, cv::Point(0, 0), cv::Point(maxX, gray.rows), CLR_BLACK, -1);
		else
			cv::rectangle(gray, cv::Point(minX, 0), cv::Point(gray.cols, gray.rows), CLR_BLACK, -1);

	}

#ifndef NO_GUI
	if (config.DRAW && config.SHOW_GRAY) {
		//frames_mtx[aIndex].lock();
		grays_to_show[aIndex] = gray.clone();
		//frames_mtx[aIndex].unlock();
	}
#endif

	ContData data[config.DATA_SIZE];

	int imgWidth = imgColor.cols;
	int imgHeight = imgColor.rows;
	int imgOffset = imgHeight / config.NUM_ROI;
	int imgOffsetV = imgWidth / config.NUM_ROI_V;

	parse_result.fl_err_line = false;

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

	parse_result.res_points.clear();
	parse_result.res_points_mm.clear();

	vector<RectData*> buf_points;
	vector<RectData*> buf_rd;

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
			//
//			cv::putText(imgColor, std::to_string(rd->bound.width), rd->center + cv::Point(0, 20),
//				cv::FONT_HERSHEY_DUPLEX, 0.5, CLR_GREEN);
		}
#endif
	}

	sort(buf_points.begin(), buf_points.end(),
		[](RectData* a, RectData* b) { return (a->bound.y > b->bound.y); });

	//	поиск горизонтальных пересечений
	find_horizontal(imgColor, buf_points, parse_result.hor_ys);
	//
	parse_result.hor_ys_mm.clear();
	for (size_t i = 0; i < parse_result.hor_ys.size(); i++) {
		CalibPoint cb = get_calib_point(imgColor, Point2f(imgColor.cols / 2, parse_result.hor_ys[i]));
		parse_result.hor_ys[i] = cb.point_cnt.y;
		parse_result.hor_ys_mm.push_back(cb.point_mm.y);
	}

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
					//&& check_rects_adj_vert(rd1->bound, rd2->bound)		//	если области пересекаются по вертикали
					//&& check_rects_adj_horz(rd1->bound, rd2->bound)		//	если области пересекаются по горизонтали
					)
				{
					double len = length(rd1->center, rd2->center);
					if (len <= min_len) {
						min_len = len;
						k = j;
					}
				}
			}
			if (k > 0) {
				buf_rd.push_back(buf_points[k]);
				//
				CalibPoint cb = get_calib_point(imgColor, buf_points[k]->center);
				parse_result.res_points.push_back(cb.point_cnt);
				parse_result.res_points_mm.push_back(cb.point_mm);
			}
			//else break;	//	если не можем построить следующий отрезок, то прекращаем обработку (сигнализировать об ошибке?)
			i = (k > 0) ? k : (i + 1);
		}
	}

	int y0_x = 10000;
	for (size_t i = 1; i < parse_result.res_points.size(); i++) {
		Point pt1 = parse_result.res_points[i - 1];
		Point pt2 = parse_result.res_points[i];
		if (pt1.y == 0) y0_x = pt1.x;
		else if (pt2.y == 0) y0_x = pt2.x;
		else if ((pt1.y < 0) && (pt2.y > 0))
			y0_x = ((pt2.x - pt1.x) * (0 - pt1.y) / (pt2.y - pt1.y)) + pt1.x;
		//
		if (y0_x != 10000) {
			parse_result.center_x = y0_x;
			CalibPoint res{ Point(0, 0), Point(y0_x, 0) };
			find_point_mm(res);
			//
			parse_result.center_x_mm = res.point_mm.x;
			break;
		}
	}

	parse_result.fl_err_line = parse_result.fl_err_line ||
		(parse_result.res_points.size() < (size_t)(config.NUM_ROI));

#ifndef NO_GUI
	if (config.DRAW) {

		for (size_t i = 0; i < buf_rd.size(); i++) {
			cv::Point c(buf_rd[i]->center);
			cv::Point c_img(imgColor.cols / 2, buf_rd[i]->center.y);
			if (config.DRAW_DETAILED)
				cv::line(imgColor, c, c_img, CLR_YELLOW, 1, cv::LINE_AA, 0);
		}

		if (buf_rd.size() > 2) {
			cv::Rect r(buf_rd[1]->bound);
			cv::Point c(buf_rd[1]->center);
			cv::Point pt1(r.x, c.y);			//	r.y + r.height
			cv::Point pt2(r.x + r.width, c.y);	//	r.y + r.height
			//
			if (config.DRAW_DETAILED) {
				cv::rectangle(imgColor, r, CLR_CYAN);
				cv::line(imgColor, pt1, pt2, CLR_MAGENTA, 1, cv::LINE_AA, 0);
			}
		}

		//

		if (parse_result.fl_err_line) {
			cv::circle(imgColor, cv::Point(50, 50), 20, CLR_RED, -1, cv::LINE_AA);
		}
		else {
			for (size_t i = 0; i < parse_result.hor_ys.size(); i++) {
				cv::Point pt_draw = point_cnt_to_topleft(imgColor, Point2f(0, parse_result.hor_ys[i]));
				pt_draw.x = 0;
				//
				cv::line(imgColor, cv::Point(0, pt_draw.y), cv::Point(imgWidth, pt_draw.y),
					CLR_RED, 2, cv::LINE_AA, 0);
				//
				cv::putText(imgColor,
					to_string(parse_result.hor_ys[i]) + " px",
					pt_draw + cv::Point(5, 20),
					cv::FONT_HERSHEY_SIMPLEX, 0.4, CLR_YELLOW);
				//
				cv::putText(imgColor,
					to_string(parse_result.hor_ys_mm[i]) + " mm",
					pt_draw + cv::Point(5, 35),
					cv::FONT_HERSHEY_SIMPLEX, 0.4, CLR_YELLOW);
			}
			//
			cv::Point line_pt(imgWidth / 2, imgHeight);
			for (size_t i = 0; i < parse_result.res_points.size(); i++) {
				cv::Point pt_src = parse_result.res_points[i];
				cv::Point pt_src_mm = parse_result.res_points_mm[i];
				cv::Point pt_draw = point_cnt_to_topleft(imgColor, pt_src);
				//
				cv::line(imgColor, line_pt, pt_draw, CLR_GREEN, 2, cv::LINE_AA, 0);
				cv::circle(imgColor, pt_draw, 3, CLR_YELLOW, -1, cv::LINE_AA);
				//
				cv::putText(imgColor,
					"(" + to_string(pt_src.x) + ";" + to_string(pt_src.y) + ") px",
					pt_draw + cv::Point(5, 20),
					cv::FONT_HERSHEY_SIMPLEX, 0.4, CLR_YELLOW);
				//
				cv::putText(imgColor,
					"(" + to_string(pt_src_mm.x) + ";" + to_string(pt_src_mm.y) + ") mm",
					pt_draw + cv::Point(5, 35),
					cv::FONT_HERSHEY_SIMPLEX, 0.4, CLR_YELLOW);
				//
				line_pt = pt_draw;
			}

			if (y0_x != 10000) {
				cv::Point pt_draw = point_cnt_to_topleft(imgColor, Point(parse_result.center_x, 0));
				cv::circle(imgColor, pt_draw, 3, CLR_MAGENTA, -1, cv::LINE_AA);
				cv::putText(imgColor,
					to_string(parse_result.center_x) + " px",
					pt_draw + cv::Point(5, 20),
					cv::FONT_HERSHEY_SIMPLEX, 0.4, CLR_MAGENTA);
				//
				cv::putText(imgColor,
					to_string(parse_result.center_x_mm) + " mm",
					pt_draw + cv::Point(5, 35),
					cv::FONT_HERSHEY_SIMPLEX, 0.4, CLR_MAGENTA);
			}
		}
		//
		//frames_mtx[aIndex].lock();
		frames_to_show[aIndex] = imgColor.clone();
		//frames_mtx[aIndex].unlock();
	}
#endif

}
