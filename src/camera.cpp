/*
 * camera.cpp
 *
 *  Created on: Aug 17, 2023
 *      Author: vevdokimov
 */
#include <unistd.h>

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
#include "barcode.hpp"
#include "templates.hpp"
#include "shared_memory.hpp"
#include "calibration.hpp"
#include "udp.hpp"
#include "camera.hpp"

cv::Mat sources_to_show[CAM_COUNT];
cv::Mat undistorteds_to_show[CAM_COUNT];
cv::Mat frames_to_show[CAM_COUNT];
cv::Mat grays_to_show[CAM_COUNT];

ResultFixed parse_results[CAM_COUNT];

DebugFixed parse_debug[CAM_COUNT];

const string SOURCE_WND_NAME = "Camera(s) Source";
const string UNDISTORTED_WND_NAME = "Camera(s) Undistorted";
const string GRAY_WND_NAME = "Camera(s) Gray";
const string COLOR_WND_NAME = "Camera(s) Color";
const string CALIBRATION_WND_NAME = "Calibration editor";

cv::Scalar templates_clr[] { CLR_YELLOW, CLR_MAGENTA, CLR_CYAN, CLR_BLUE, CLR_GREEN };

const int SHOW_GRAY = 1;

int16_t hidro_height[CAM_COUNT];

bool check_rects_adj_horz(const cv::Rect r1, const cv::Rect r2)
{
	// Если один прямоугольник находится справа от другого
	if ((r1.x > (r2.x + r2.width)) ||
		(r2.x > (r1.x + r1.width))) {
		return false;
	}
	// В противном случае, прямоугольники пересекаются
	return true;
}

void visualizer_func()
{
	pthread_setname_np(pthread_self(), "visualizer thread");

	write_log("visualizer_func() started!");
	write_log("visualizer_func() entered infinity loop.");

	while (!kill_threads)
	{
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
			if (!(sources_to_show[i].empty()))
				source = sources_to_show[i].clone();
			if (!(undistorteds_to_show[i].empty()))
				undistorted = undistorteds_to_show[i].clone();
			if (SHOW_GRAY && !(grays_to_show[i].empty()))
				gray = grays_to_show[i].clone();
			if (!(frames_to_show[i].empty()))
				frame = frames_to_show[i].clone();
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
		if (SHOW_GRAY && (grays.size() > 0))
			cv::hconcat(grays, mergedGray);
		if (frames.size() > 0)
			cv::hconcat(frames, mergedFrames);
		//
//		if (!mergedSource.empty())
//			cv::imshow(SOURCE_WND_NAME, mergedSource);
//		if (!mergedUndistorted.empty())
//			cv::imshow(UNDISTORTED_WND_NAME, mergedUndistorted);
		if (SHOW_GRAY && !mergedGray.empty())
			cv::imshow(GRAY_WND_NAME, mergedGray);
		if (!mergedFrames.empty())
			cv::imshow(COLOR_WND_NAME, mergedFrames);
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

void* p_visualizer_func(void *args) { visualizer_func(); return 0; }

void camera_func(string aThreadName, string aCamAddress, int aIndex)
{

	pthread_setname_np(pthread_self(), aThreadName.c_str());

	// получение ID текущего потока
	pthread_t tid = pthread_self();
	// объявление структуры cpu_set_t для хранения маски ядер
	cpu_set_t cpuset;
	// инициализация структуры cpu_set_t
	CPU_ZERO(&cpuset);
	// установка маски ядер (в данном случае - первое ядро)
	CPU_SET(aIndex, &cpuset);
	// установка маски ядер для текущего потока
	if (int res = pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset))
	{
		write_log(aThreadName + " pthread_setaffinity_np ERROR = " + to_string(res));
	}

    write_log(aThreadName + " started!");

	clock_t tStart;

	double tt, tt_sum, tt_min, tt_max;
	int tt_cnt = 0;
	int incorrect_lines = 0;

	uint64_t read_err = 0;

	cv::VideoCapture cap;
	cv::Mat frame;
	cv::Mat undistorted;

	ParseImageResult parse_result;

	parse_result.fl_err_camera = true;	//	инициализация камеры

	double tt_elapsed, tt_prev = clock();
	int fps_count = 0;

	bool err_open, err_grab, err_empty;

	bool camera_open_error_logged = false;
	bool camera_read_error_logged = false;

	bool slow_stop_found = false;

#ifdef USE_FILE

	frame = imread(FILE_PATH, IMREAD_COLOR);
	//
	if (frame.empty()) return;
	//
	parse_result.fl_err_camera = false;

#endif

	while (1)
	{
		if (restart_threads || kill_threads) break;

#ifndef USE_FILE

		if (parse_result.fl_err_camera) {

			if (cap.isOpened())
				cap.release();

			parse_result.fl_err_camera = !cap.open(aCamAddress);
			parse_result_to_sm(parse_result, aIndex);

			if (parse_result.fl_err_camera)
			{
				if (!camera_open_error_logged)
				{
					write_log(aThreadName + " - error opening camera!");
					camera_open_error_logged = true;
				}
				continue;
			}

			camera_open_error_logged = false;

			//	очищаем буфер
			for (int i = 0; i < CLEAR_CAM_BUFFER; i++)
				cap.grab();

			write_log(aThreadName + " entered infinity loop.");

		}

		err_open = !cap.isOpened();

		if (!err_open)
			err_grab = !cap.grab();

		if (config.USE_FPS && (config.FPS > 0))
		{
			if (!err_open && !err_grab)
			{
				fps_count++;
				tt_elapsed = (double)(clock() - tt_prev) / CLOCKS_PER_SEC;
				if (tt_elapsed < (1. / config.FPS))
					continue;

				fps_count = 0;
				tt_prev = clock();
			}
		}

		if (!err_open && !err_grab)
			cap.retrieve(frame);
		err_empty = frame.empty();

		parse_result.fl_err_camera = err_open || err_grab || err_empty;

		if (parse_result.fl_err_camera)
		{
			if (!camera_read_error_logged)
			{
				write_log(aThreadName + " read error! " +
					to_string(err_open) + " " +
					to_string(err_grab) + " " +
					to_string(err_empty)
				);
				camera_read_error_logged = true;
			}
			parse_result_to_sm(parse_result, aIndex);
			continue;
		}

		camera_read_error_logged = false;

#endif	// USE_FILE

		tStart = clock();

		try
		{
#ifdef USE_UNDISTORT
			undistort(frame, undistorted, cameraMatrix, distCoeffs);
#else
			undistorted = frame.clone();
#endif
		}
		catch (...)
		{
			parse_result.fl_err_camera = true;
			write_log(aThreadName + " undistortion error!");
			parse_result_to_sm(parse_result, aIndex);
			continue;
		}

		sources_to_show[aIndex] = frame.clone();
		undistorteds_to_show[aIndex] = undistorted.clone();

		parse_result.width = frame.cols;
		parse_result.height = frame.rows;

		try {

			//	ищем центры областей и горизонтальные пересечения
			parse_image(
				aThreadName,
				undistorted,
				parse_result,
				aIndex,
				slow_stop_found
			);
			parse_result.fl_err_parse = false;

			slow_stop_found =
				parse_result.fl_slow_zone ||
				parse_result.fl_stop_zone ||
				parse_result.fl_stop_mark;

		} catch (...) {
			parse_result.fl_err_parse = true;
		}

		parse_result_to_sm(parse_result, aIndex);
		if (config.WEB_SHOW_DEBUG || config.WEB_SHOW_IMAGE)
			parse_debug_to_sm(parse_debug[aIndex], aIndex);

		if (parse_result.fl_err_parse) {
			write_log(aThreadName + " parse error!");
			continue;
		}

		if (config.STATS_LOG && (config.STATS_COUNT > 0))
		{
			if (parse_result.fl_err_line)
				incorrect_lines++;
			//
			tt = (double)(clock() - tStart) / CLOCKS_PER_SEC;
			if (!tt_cnt) { tt_sum = 0; tt_max = tt; tt_min = tt; }
			//
			if (tt > tt_max) tt_max = tt;
			if (tt < tt_min) tt_min = tt;
			tt_sum += tt;
			tt_cnt++;
			//
			if (tt_cnt >= config.STATS_COUNT)
			{
//				if (read_err)
//					write_log(aThreadName + " reading errors = " + to_string(read_err));
//				write_log(aThreadName + " incorrect lines: " + to_string(incorrect_lines));
				write_log(aThreadName + " time taken:" +
					" min=" + to_string(tt_min) +
					" max=" + to_string(tt_max) +
					" avg=" + to_string(tt_sum / tt_cnt));
				//
				incorrect_lines = 0;
				tt_cnt = 0;
			}
		}

		usleep(1000);

	}

	if (cap.isOpened())
		cap.release();

	write_log(aThreadName + " is out of infinity loop.");

}

void find_barcodes(cv::Mat& imgColor, ParseImageResult& parse_result,
	std::vector<BarcodeDetectionResult>& barcodes_results,
	const bool slow_stop_found)
{
	barcodes_detect(imgColor, barcodes_results, slow_stop_found);
	//
	for (size_t i = 0; i < barcodes_results.size(); i++)
	{
		BarcodeDetectionResult bdr = barcodes_results[i];

#ifdef BARCODE_LOG
		write_log("barcode found! " + bdr.text);
#endif
		cv::Moments M = cv::moments(bdr.contour);
		cv::Point2f center(M.m10 / M.m00, M.m01 / M.m00);

	#ifndef NO_GUI
		const auto* pts = bdr.contour.data();
		int npts = bdr.contour.size();

		cv::circle(imgColor, center, 3, CLR_YELLOW, -1, cv::LINE_AA);
		//
		cv::polylines(imgColor, &pts, &npts, 1, true, CLR_GREEN);
		cv::putText(imgColor, bdr.text, bdr.contour[3] + cv::Point(0, 20),
			cv::FONT_HERSHEY_DUPLEX, 0.5, CLR_GREEN);
	#endif

		if (bdr.barcode_type == 3) // DataMatrix
		{
			parse_result.fl_stop_mark = true;
			//
			cv::Point2f p1(center.x, imgColor.rows / 2);
			double dist1 = get_points_distance(bdr.contour[1], bdr.contour[2]);
			double dist2 = get_points_distance(center, p1) * (config.DATAMATRIX_WIDTH / dist1);

			if (center.y > imgColor.rows / 2) dist2 *= -1;

	#ifndef NO_GUI
			cv::line(imgColor, p1, center, CLR_YELLOW, 1, cv::LINE_AA, 0);
			cv::putText(imgColor, std::to_string(dist2), bdr.contour[3] + cv::Point(0, 40),
				cv::FONT_HERSHEY_DUPLEX, 0.5, CLR_GREEN);
	#endif

			parse_result.stop_mark_distance = round(dist2);
		}
		//
		else if (bdr.barcode_type == 1)
			parse_result.fl_slow_zone = true;
		else if (bdr.barcode_type == 2)
			parse_result.fl_stop_zone = true;
	}
}

void hide_barcodes(cv::Mat& gray, std::vector<BarcodeDetectionResult> barcodes_results)
{
	int minX = gray.cols;
	int maxX = 0;
	for (size_t i = 0; i < barcodes_results.size(); i++)
		for (size_t j = 0; j < barcodes_results[i].contour.size(); j++) {
			cv::Point point = barcodes_results[i].contour[j];
			if (point.x < minX) minX = point.x;
			if (point.x > maxX) maxX = point.x;
		}
	//
	for (size_t i = 0; i < barcodes_results.size(); i++)
		cv::rectangle(gray, cv::boundingRect(barcodes_results[i].contour), CLR_BLACK, -1);
	//
	if (barcodes_results.size() > 0)
	{
		if (config.BARCODE_LEFT)
			cv::rectangle(gray, cv::Point(0, 0), cv::Point(maxX, gray.rows), CLR_BLACK, -1);
		else
			cv::rectangle(gray, cv::Point(minX, 0), cv::Point(gray.cols, gray.rows), CLR_BLACK, -1);
	}
}

void find_templates(cv::Mat& imgColor, ParseImageResult& parse_result,
	std::vector<TemplateDetectionResult>& templates_results)
{

	templates_detect(imgColor, templates_results);

#ifndef NO_GUI

	Scalar clr;
	for (size_t i = 0; i < templates.size(); i++)
	{
		clr = templates_clr[ i % (sizeof(templates_clr) / sizeof(templates_clr[0])) ];
		rectangle(imgColor,
			templates[i].roi.tl(),
			templates[i].roi.br(),
			clr, 2, 8, 0);
	}

#endif

	for (size_t i = 0; i < templates_results.size(); i++)
	{
		TemplateDetectionResult tdr = templates_results[i];

#ifdef BARCODE_LOG
		write_log("template_" + to_string(tdr.template_id) + " found! " + to_string(tdr.match));
#endif

		cv::Point2f center(
			tdr.found_rect.x + tdr.found_rect.width / 2,
			tdr.found_rect.y + tdr.found_rect.height / 2
		);

		if (tdr.template_id == 0)
			parse_result.fl_slow_zone = true;
		else if (tdr.template_id == 1)
			parse_result.fl_stop_zone = true;
		else if (tdr.template_id == 2)
		{
			parse_result.fl_stop_mark = true;

			cv::Point2f p1(center.x, imgColor.rows / 2);
			double dist1 = tdr.found_rect.width;
			double dist2 = get_points_distance(center, p1) * (config.DATAMATRIX_WIDTH / dist1);

#ifndef NO_GUI
			cv::line(imgColor, p1, center, CLR_YELLOW, 1, cv::LINE_AA, 0);
			cv::putText(imgColor, std::to_string(dist2), center,
				cv::FONT_HERSHEY_DUPLEX, 0.5, CLR_GREEN);
#endif

			parse_result.stop_mark_distance = round(dist2);
		}

#ifndef NO_GUI

		clr = templates_clr[ tdr.template_id % (sizeof(templates_clr) / sizeof(templates_clr[0])) ];

		cv::circle(imgColor, center, 3, clr, -1, cv::LINE_AA);
		//
		rectangle(imgColor,
			tdr.found_rect.tl(),
			tdr.found_rect.br(),
			clr, 2, 8, 0
		);
		putText(imgColor,
			"templ_" + to_string(tdr.template_id),
			tdr.found_rect.tl() + Point(10, 15),
			cv::FONT_HERSHEY_SIMPLEX, 0.4, clr, 1
		);
		putText(imgColor,
			to_string(tdr.match),
			tdr.found_rect.tl() + Point(10, 30),
			cv::FONT_HERSHEY_SIMPLEX, 0.4, clr, 1
		);

#endif

	}
}

void apply_filters(cv::Mat& srcImg, cv::Mat& dstImg, int aIndex)
{
	cv::Mat trImage(srcImg.rows, srcImg.cols, CV_8U);
	cv::cvtColor(srcImg, trImage, cv::COLOR_BGR2GRAY);

	int gbk = config.GAUSSIAN_BLUR_KERNEL;
	cv::GaussianBlur(trImage, trImage, Size2i(gbk, gbk), 0);

	int mok = config.MORPH_OPEN_KERNEL;
	cv::morphologyEx(trImage, trImage, MORPH_OPEN, getStructuringElement(MORPH_RECT, Size2i(mok, mok)));

	int mck = config.MORPH_CLOSE_KERNEL;
	cv::morphologyEx(trImage, trImage, MORPH_CLOSE, getStructuringElement(MORPH_RECT, Size2i(mck, mck)));

	cv::threshold(trImage, dstImg,
			config.THRESHOLD_THRESH + hidro_height[aIndex] * ( config.THRESHOLD_HEIGHT_K / 100. ),
			config.THRESHOLD_MAXVAL, cv::THRESH_BINARY_INV);
}

bool find_central_point(ParseImageResult& parse_result)
{
	int y0_x = 10000;
	bool found = false;
	for (size_t i = 1; i < parse_result.res_points.size(); i++)
	{
		//	берем соседние точки линии;
		//	если у какой-то из них y = 0, то она и есть центральная;
		//	если точки находятся по разные стороны от оси X,
		//	то берем уравнение прямой, проходящей через них и находим значение x при y = 0
		ResultPoint pt1 = parse_result.res_points[i - 1];
		ResultPoint pt2 = parse_result.res_points[i];
		if (pt1.y == 0) y0_x = pt1.x;
		else if (pt2.y == 0) y0_x = pt2.x;
		else if ((pt1.y < 0) && (pt2.y > 0))
			y0_x = ((pt2.x - pt1.x) * (0 - pt1.y) / (pt2.y - pt1.y)) + pt1.x;
		//
		if (y0_x != 10000)
		{
			found = true;
			break;
		}
	}

	//	если нет точки, пересекающей y = 0, то берем х первой точки
	if (!found && (parse_result.res_points.size() >= 2))
	{
		ResultPoint pt1 = parse_result.res_points[0];
		ResultPoint pt2 = parse_result.res_points[parse_result.res_points.size() - 1];
		y0_x = (pt1.x + pt2.x) / 2;
		found = true;
	}

	if (found)
	{
		parse_result.center_x = y0_x;
		//
		CalibPoint res{ Point(0, 0), Point(y0_x, 0) };
		find_point_mm(res);
		parse_result.center_x_mm = res.point_mm.x;
	}

	return found;
}

void compress_image(const Mat src, uchar* dst_arr, int16_t* dst_arr_sz)
{
	uchar max_val = config.THRESHOLD_MAXVAL;
	uchar pix, pix_p;
	int k = 0, m = 0;
	for (int i = 0; i < src.rows; i++)
		for (int j = 0; j < src.cols; j++)
		{
			pix = (src.at<uchar>(i, j) == max_val) ? 1 : 0;
			m++;
			//
			if (!j) { pix_p = pix; }
			//
			if ((pix != pix_p)			//	если пиксел отличается от предыдущего
				|| (m == 127)			//	или счётчик считаемых битов = 127
				|| (j == src.cols - 1)	//	или закончилась строка
				)
			{
				dst_arr[k++] = ((m & 127) << 1) | (pix_p & 1);
				//
				pix_p = pix;
				m = 0;
				//
				if (k >= DEBUG_MAX_IMG_SIZE)
				{
					*dst_arr_sz = DEBUG_MAX_IMG_SIZE;
					return;
				}
			}
		}
	*dst_arr_sz = k;
}

void decompress_image(const uchar* src_arr, const int16_t src_arr_sz, Mat& dst)
{
	int i = 0, j = 0;
	uchar max_val = config.THRESHOLD_MAXVAL;
	//
	for (int m = 0; m < src_arr_sz; m++)
		for (int n = 0; n < (src_arr[m] >> 1); n++)
		{
			dst.at<uchar>(i, j++) = (src_arr[m] & 1) ? max_val : 0;
			//
			if (j == dst.cols)
			{
				i++;
				j = 0;
			}
		}
}

void set_contour_type_by_bound(DebugContourInfo* arr, int arr_size, Rect rect, int type)
{
	for (int i = 0; i < arr_size; ++i)
	{
		Rect r(arr[i].left_top.x, arr[i].left_top.y, arr[i].width, arr[i].height);
		if (r == rect) arr[i].type = type;
	}
}

void parse_image(string aThreadName, cv::Mat imgColor,
	ParseImageResult& parse_result, int aIndex,
	const bool slow_stop_found)
{
	parse_result.fl_slow_zone = false;
	parse_result.fl_stop_zone = false;
	parse_result.fl_stop_mark = false;
	parse_result.stop_mark_distance = 0;

	parse_result.pult_flags = udp_request.pult_flags | config.AUTO_EMULATE;
	//
	hidro_height[aIndex] = udp_request.hidro_height[aIndex];
	parse_result.hidro_height = hidro_height[aIndex];

#ifdef USE_BARCODES
	//	поиск штрихкодов
	std::vector<BarcodeDetectionResult> barcodes_results;
	find_barcodes(imgColor, parse_result, barcodes_results, slow_stop_found);
#endif

#ifdef USE_TEMPLATES
	//	поиск шаблонов
	std::vector<TemplateDetectionResult> templates_results;
	find_templates(imgColor, parse_result, templates_results);
#endif

	//	применение фильтров
	cv::Mat gray;
	apply_filters(imgColor, gray, aIndex);

#ifdef USE_BARCODES
	//	сокрытие штрихкодов
	//	hide_barcodes(gray, barcodes_results);
#endif

	//	сжатие изображения в массив
	if (config.WEB_SHOW_IMAGE)
	{
		compress_image(gray, parse_debug[aIndex].image, &(parse_debug[aIndex].image_size));
	}

	//	config.NUM_ROI - это общее количество ROI на изображении;
	ContData data[config.NUM_ROI];

	int imgWidth = imgColor.cols;
	int imgHeight = imgColor.rows;
	int imgOffset = imgHeight / config.NUM_ROI;

	parse_result.fl_err_line = false;

	//	разбиваем изображение на ROI,
	//	находим контуры и параметры контуров в них
	for (int i = 0; i < config.NUM_ROI; ++i)
	{
		cv::Rect roi(0, imgOffset * i, imgWidth, imgOffset);
		get_contur_params(gray, roi, data[i], i, 0);
	}

	cv::Point pt(imgWidth / 2, imgHeight);
	cv::Point* lpCenter = &pt;

	parse_result.res_points.clear();

	vector<RectData*> buf_points;

	int csz = 0;

	//	ищем в каждом ROI прямоугольник,
	//	центр которого находится ближе всего к базовой точке
	for (int i = 0; i < config.NUM_ROI; ++i)
	{
		ContData* dt = &data[config.NUM_ROI - 1 - i];

		for (int j = 0; j < (int)dt->vRect.size(); j++)
		{
			if (csz >= DEBUG_MAX_CONTOURS) break;
			//
			RectData rd = dt->vRect[j];
			DebugContourInfo dci;
			//
			dci.type = 1;	// все контуры
			dci.center = { rd.center.x, rd.center.y };
			dci.left_top = { rd.bound.tl().x, rd.bound.tl().y };
			dci.width = rd.bound.width;
			dci.height = rd.bound.height;
			dci.length = rd.len;
			//
			parse_debug[aIndex].contours[csz++] = dci;
		}

		//	сортируем прямоугольники, обрамляющие контуры, по близости к базовой точке;
		//	при первой итерации, базовая точка - центр нижнего края изображения
		RectData* rd = sort_cont(*lpCenter, dt[0]);

		if (rd == nullptr)
			continue;

		//	выделяем выбранную в ROI область
		set_contour_type_by_bound(parse_debug[aIndex].contours,
			parse_debug[aIndex].contours_size, rd->bound, 2);

		buf_points.push_back(rd);
		//
		//	центр найденного прямоугольника становится новой базовой точкой
		lpCenter = &rd->center;

	}

	parse_debug[aIndex].contours_size = csz;

	bool is_auto = (parse_result.pult_flags & 1);

	//	фильтруем список областей, оставляя только те,
	//	у которых есть смежные по вертикали в соседних ROI
	{
		size_t i = 0;
		RectData* rd1;
		RectData* rd2;
		bool fl = false;
		while (i < buf_points.size())
		{
			rd1 = buf_points[i];
			fl = false;
			for (size_t j = 0; j < buf_points.size(); j++)
			{
				if (i == j) continue;
				//
				rd2 = buf_points[j];
				if (true
					//	если области в соседних строках
					&& (!config.FILTER_NEIGHBOR || (config.FILTER_NEIGHBOR && (abs(rd1->roi_row - rd2->roi_row) == 1)))
					//	если области пересекаются по горизонтал
					&& check_rects_adj_horz(rd1->bound, rd2->bound)
				)
				{
					fl = true;
					break;
				}
			}
			//
			if (fl) {
				i++;
			} else {
				buf_points.erase(buf_points.begin() + i);
				//	выделяем удаленную область, не подошедшую по условиям
				set_contour_type_by_bound(parse_debug[aIndex].contours,
					parse_debug[aIndex].contours_size, rd1->bound, 3);
			}
		}
	}

	//	добавляем нижнюю центральную точку в список для построения линии
	RectData center_rd;
	center_rd.center = cv::Point(imgWidth / 2, imgHeight);
	center_rd.bound = cv::Rect(0, imgHeight, imgWidth, 10);
	center_rd.roi_row = config.NUM_ROI;
	buf_points.insert(buf_points.begin(), &center_rd);

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
				//
				if ((i == 0) || //	центральная точка соединяется с любой ближайшей
					(
						true
						//	если области в соседних строках
						&& (!config.FILTER_NEIGHBOR || (config.FILTER_NEIGHBOR && (abs(rd1->roi_row - rd2->roi_row) == 1)))
						//	если области пересекаются по горизонтали
						&& check_rects_adj_horz(rd1->bound, rd2->bound)
					)
				)
				{
					double len = length(rd1->center, rd2->center);
					if (len <= min_len) {
						min_len = len;
						k = j;
					}
				}
			}
			//
			if (k > 0)
			{
				CalibPoint cb = get_calib_point(imgColor, buf_points[k]->center);
				parse_result.res_points.push_back(ResultPoint{cb.point_cnt.x, cb.point_cnt.y});
			}
			//
			i = (k > 0) ? k : (i + 1);
		}
	}

	//	находим центральную точку и её координаты в мм
	bool y0x_found = find_central_point(parse_result);

	int points_count = parse_result.res_points.size();

	//	ошибка поиска линии
	parse_result.fl_err_line =
		//	если не удалось найти центральную точку
		!y0x_found
		//	или количество точек в меньше минимального в ручном режиме
		|| (!is_auto && (points_count < config.MANUAL_MIN_POINTS))
		//	или количество точек в меньше минимального в режиме АВТО
		|| (is_auto && (points_count < config.AUTO_MIN_POINTS));

#ifndef NO_GUI

	if (SHOW_GRAY)
	{
		grays_to_show[aIndex] = gray.clone();
	}

	//	рисуем сетку из ROI
	for (int i = 0; i < config.NUM_ROI; ++i)
		cv::rectangle(imgColor, data[i].roi, CLR_YELLOW, 1, cv::LINE_AA, 0);

	if (config.USE_IMAGE_ROI)
	{
		cv::rectangle(
			imgColor,
			cv::Rect(
				config.IMAGE_ROI_X,
				config.IMAGE_ROI_Y,
				config.IMAGE_ROI_W,
				config.IMAGE_ROI_H
			),
			CLR_CYAN
		);
	}

	//	рисуем ВСЕ найденные области в ROI
	for (int i = 0; i < parse_debug[aIndex].contours_size; i++)
	{
		DebugContourInfo dci = parse_debug[aIndex].contours[i];
		//
		Scalar clr;
		switch (dci.type)
		{
			case 2:
				clr = CLR_YELLOW;
				break;
			case 3:
				clr = CLR_BLACK;
				break;
			default:
				clr = CLR_RECT_BOUND;
		}
		//
		cv::rectangle(imgColor, cv::Rect(dci.left_top.x, dci.left_top.y, dci.width, dci.height), clr);
		cv::circle(imgColor, Point(dci.center.x, dci.center.y), 3, CLR_GREEN, 1, cv::LINE_AA);
		//
		cv::putText(imgColor, "L=" + std::to_string(dci.length), Point(dci.left_top.x, dci.left_top.y) + cv::Point(2, 12),
			cv::FONT_HERSHEY_DUPLEX, 0.4, CLR_GREEN);
		cv::putText(imgColor, "W=" + std::to_string(dci.width), Point(dci.left_top.x, dci.left_top.y) + cv::Point(2, 27),
			cv::FONT_HERSHEY_DUPLEX, 0.4, CLR_GREEN);
	}

	//	рисуем центральные линии
	cv::Point cnt(imgColor.cols / 2, imgColor.rows / 2);
	cv::line(imgColor, cv::Point2f(cnt.x, 0), cv::Point2f(cnt.x, imgColor.rows),
		CLR_MAGENTA, 1, cv::LINE_8, 0);
	cv::line(imgColor, cv::Point2f(0, cnt.y), cv::Point2f(imgColor.cols, cnt.y),
		CLR_MAGENTA, 1, cv::LINE_8, 0);

	if (parse_result.fl_err_line)
	{
		//	сигнализируем об ошибке
		cv::circle(imgColor, cv::Point(50, 50), 20, CLR_RED, -1, cv::LINE_AA);
	}
	else
	{
		//	рисуем линии горизонтальных пересечений
		for (size_t i = 0; i < parse_result.hor_ys.size(); i++)
		{
			cv::Point pt_draw = point_cnt_to_topleft(imgColor, Point2f(0, parse_result.hor_ys[i]));
			pt_draw.x = 0;
			//
			cv::line(imgColor, cv::Point(0, pt_draw.y), cv::Point(imgWidth, pt_draw.y),
				CLR_RED, 2, cv::LINE_AA, 0);
			//
			cv::putText(imgColor,
				to_string(parse_result.hor_ys[i]) + " px",
				pt_draw + cv::Point(5, 20),
				cv::FONT_HERSHEY_SIMPLEX, 0.4, CLR_RED);
		}
		//
		//	рисуем основную линию
		cv::Point line_pt(imgWidth / 2, imgHeight);
		for (size_t i = 0; i < parse_result.res_points.size(); i++)
		{
			ResultPoint mpt = parse_result.res_points[i];
			cv::Point pt_src(mpt.x, mpt.y);
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
			line_pt = pt_draw;
		}
		//
		//	рисуем центральную точку
		if (y0x_found)
		{
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
	cv::putText(imgColor,
			"hidro_height = " + to_string(hidro_height[aIndex]) + " mm",
			cv::Point(130, 20),
			cv::FONT_HERSHEY_DUPLEX, 0.5, CLR_MAGENTA);
	//
	frames_to_show[aIndex] = imgColor.clone();

#endif

}
