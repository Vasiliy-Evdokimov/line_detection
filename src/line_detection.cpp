#include "opencv2/opencv.hpp"

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time.h>

#define _USE_MATH_DEFINES
#include <math.h>

// UDP data
/*
#include "Ws2tcpip.h"
#include <WinSock2.h>
#include <iostream>
#include <tchar.h>
#pragma comment (lib, "Ws2_32.lib")

#include "crc.h"
*/

using namespace cv;
using namespace std;

#define ROI_IMAGE	0
#define NUM_ROI		4

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

#define IMG_ADDR "D:\\projects\\opencv_cpp\\my_test_opencv\\x64\\Debug\\image.png"
#define CAM_ADDR "rtsp://admin:1234qwer@192.168.1.64:554/streaming/channels/2"

#define UDP_ADDR "127.0.0.1"
#define UDP_PORT 6000

#define USE_CAMERA	1
#define DETAILED	0
#define SHOW_GRAY	0
#define SHOW_LINES	1
#define DRAW		1
#define MORPHOLOGY	1

struct RectData
{
	int dir;
	int _reserved;
	double len;
	cv::Point center;
	cv::Rect bound;
	double angle;
};

struct ContData
{
	std::vector<RectData> vRect;
};

void get_contur_params(cv::Mat& img, cv::Rect& roi, ContData& data, double min_cont_len)
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
	if (center == nullptr)
		return nullptr;

	if (DETAILED) {

		center->dir = DIR_FORWARD;

		for (auto i = data.vRect.begin(); i != data.vRect.end(); ++i)
		{
			double a = angle(base - i->center) - 90;

			i->angle = a;

			if (&(*i) == center)
				continue;

			i->dir = (a < 0) ? DIR_LEFT : DIR_RIGHT;
		}

	}

	return center;
}

void parse_image(cv::Mat imgColor, std::vector<cv::Point>& res_points)
{
	cv::Mat trImage(imgColor.rows, imgColor.cols, CV_8U);
	cv::cvtColor(imgColor, trImage, cv::COLOR_BGR2GRAY);

//	if (MORPHOLOGY) {
//		cv::morphologyEx(trImage, trImage, MORPH_OPEN, getStructuringElement(MORPH_RECT, Size(5, 5)));
//		cv::morphologyEx(trImage, trImage, MORPH_CLOSE, getStructuringElement(MORPH_RECT, Size(5, 5)));
//	}

	cv::Mat gray(imgColor.rows, imgColor.cols, CV_8U);
	cv::threshold(trImage, gray, 127, 255, cv::THRESH_BINARY_INV);

	ContData data[(NUM_ROI)];	//	(NUM_ROI - 1)
	char buff[30];

	int imgWidth = imgColor.cols;
	int imgHeight = imgColor.rows;
	int imgOffset = imgHeight / NUM_ROI;
	int centerX = imgWidth / 2;
	int centerY = imgHeight / 2;

	for (int i = 0; i < NUM_ROI; ++i)
	{
		//	if (i < 1)
		//		continue;

		cv::Rect roi(0, imgOffset * i, imgWidth, imgOffset);

		get_contur_params(gray, roi, data[i], 100.0);	//	i - 1

		if (DETAILED && DRAW)
		{
			int ui = 0;
			for (auto u = data[i].vRect.begin(); u != data[i].vRect.end(); ++u)	//	i - 1
			{
				cv::rectangle(imgColor, u->bound, CLR_RECT_BOUND);
				cv::circle(imgColor, u->center, 3, CLR_RED, 1, cv::LINE_AA);
				//
				sprintf(buff, "%i_%i", i, ui++);
				cv::Point p(u->center.x, u->center.y - 20);
				cv::putText(imgColor, buff, p, cv::FONT_HERSHEY_COMPLEX_SMALL, 0.5, CLR_BLACK, 1, cv::LINE_AA);
			}
		}

	}

	cv::Point pt(imgColor.cols / 2, imgColor.rows);
	cv::Point* lpCenter = &pt;

	res_points.clear();

	for (int i = 0; i < (NUM_ROI); ++i)	//	(NUM_ROI - 1)
	{
		ContData* dt = &data[(NUM_ROI) - 1 - i];	//	(NUM_ROI - 1)
		RectData* rd = sort_cont(*lpCenter, dt[0]);

		if (rd == nullptr)
			break;

		if (DRAW) {
			cv::line(imgColor, *lpCenter, rd->center, CLR_GREEN, 2, cv::LINE_AA, 0);
			//
			//cv::Point pt(rd->center.x - centerX, -1 * (rd->center.y - centerY));
			cv::Point pt(rd->center.x, rd->center.y);
			res_points.push_back(pt);
			//
			sprintf(buff, "%d : %d", pt.x, pt.y);
			cv::Point p(rd->center.x, rd->center.y - 7);
			cv::putText(imgColor, buff, p, cv::FONT_HERSHEY_COMPLEX_SMALL, 0.5, CLR_RED, 1, cv::LINE_AA);
		}
		//
		/*
		sprintf_s(buff, "%0.3f", static_cast<float>(rd->angle));
		cv::Point p(rd->center.x, rd->center.y - 7);
		cv::putText(imgColor, buff, p, cv::FONT_HERSHEY_COMPLEX_SMALL, 0.5, CLR_RED, 1, cv::LINE_AA);
		*/

		if (DETAILED && DRAW)
		{
			for (const RectData& _rd : dt->vRect)
			{
				if (_rd.dir == DIR_FORWARD)
					continue;

				cv::Scalar clr = (_rd.dir == DIR_LEFT) ? CLR_BLUE : CLR_RED;

				cv::line(imgColor, *lpCenter, _rd.center, clr, 1, cv::LINE_8, 0);
				//
				/*
				sprintf_s(buff, "%0.3f", static_cast<float>(_rd.angle));
				cv::Point p(_rd.center.x, _rd.center.y - 7);
				cv::putText(imgColor, buff, p, cv::FONT_HERSHEY_COMPLEX_SMALL, 0.5, CLR_RED, 1, cv::LINE_AA);
				*/
			}
		}
		lpCenter = &rd->center;

	}

	if (SHOW_LINES && DRAW) {
		cv::line(imgColor, cv::Point(0, imgHeight / 2), cv::Point(imgWidth, imgHeight / 2), CLR_YELLOW, 1, cv::LINE_AA, 0);
		cv::line(imgColor, cv::Point(imgWidth / 2, 0), cv::Point(imgWidth / 2, imgHeight), CLR_YELLOW, 1, cv::LINE_AA, 0);
	}

	if (DRAW) cv::imshow("img", imgColor);
	if (SHOW_GRAY && DRAW) cv::imshow("gray", gray);
	//cv::imshow("thresh", trImage);
}

int main(int argc, char** argv)
{
	clock_t tStart;

	const int avg_cnt = 20;
	double tt, sum = 0;
	int cnt = 0;

	std::vector<cv::Point> res_points;

/*
	//
	// Загружаем библиотеку сокетов
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		return 0;
	}
	if (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();
		return 0;
	}

	// Создаем сокет
	SOCKET client = socket(AF_INET, SOCK_DGRAM, 0);
	SOCKADDR_IN addrSrv;
	InetPton(AF_INET, _T(UDP_ADDR), &addrSrv.sin_addr.S_un.S_addr);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(UDP_PORT);
	//
	//
*/
	if (!USE_CAMERA) {

		tStart = clock();

		cv::Mat img = cv::imread(IMG_ADDR, cv::IMREAD_COLOR);

		parse_image(img, res_points);

		tt = (double)(clock() - tStart) / CLOCKS_PER_SEC;
		printf("Time taken: %.3fs\n", tt);

		cv::waitKey();

	} else {

		cv::VideoCapture cap(CAM_ADDR);

		if (!cap.isOpened()) {
//			std::cerr << "Error opening camera." << std::endl;
			return -1;
		}

		cv::Mat frame;

		// Очищаем буфер
		for (int i = 0; i < 20; i++)
			cap >> frame;

		uint16_t counter = 0;

		char udp_pack[255];

		while (true) {

			tStart = clock();

			cap >> frame;
			parse_image(frame, res_points);

			tt = (double)(clock() - tStart) / CLOCKS_PER_SEC;
			sum += tt;
			cnt++;

			memset(udp_pack, 0, sizeof(udp_pack));

			counter++;

			udp_pack[0] = counter & 0xFF;
			udp_pack[1] = counter >> 8;

			udp_pack[2] = frame.cols & 0xFF;
			udp_pack[3] = frame.cols >> 8;
			udp_pack[4] = frame.rows & 0xFF;
			udp_pack[5] = frame.rows >> 8;

			udp_pack[6] = res_points.size();

			int pack_cnt = 7;

			for (int i = 0; i < res_points.size(); i++)
			{
				udp_pack[pack_cnt++] = res_points[i].x & 0xFF;
				udp_pack[pack_cnt++] = res_points[i].x >> 8;
				udp_pack[pack_cnt++] = res_points[i].y & 0xFF;
				udp_pack[pack_cnt++] = res_points[i].y >> 8;
			}

			// CRC
			/*
			uint16_t crc = crc16(udp_pack, pack_cnt);
			udp_pack[pack_cnt++] = (crc & 0xFF);
			udp_pack[pack_cnt++] = (crc >> 8);
			*/

			//sendto(client, udp_pack, pack_cnt, 0, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));

			if (cnt >= avg_cnt) {
				printf("Average time taken: %.3fs\n", sum / avg_cnt);
				cnt = 0;
				sum = 0;
			}

			if (cv::waitKey(1) == 27)
				break;

		}

	}

//	closesocket(client);
//	WSACleanup();

    return 0;
}
