/*
 * barcode.cpp
 *
 *  Created on: 17 авг. 2023 г.
 *      Author: vevdokimov
 */

#include <iostream>
#include <string>
#include <cmath>

#include "opencv2/opencv.hpp"

#include <zbar.h>	//	порядок важен - сначала подключаем zbar, потом zxing, иначе конфликт

#include "ZXing/ReadBarcode.h"
#include "ZXing/BarcodeFormat.h"

#include "defines.hpp"

const double FOCAL_DIST = 2.8;
const double MATRIX_W = 3.7;
const double MATRIX_H = 2.7;
const double BARCODE_W = 45;
const double DATAMATRIX_W = 26;	//	31

inline ZXing::ImageView ImageViewFromMat(const cv::Mat& image)
{
	using ZXing::ImageFormat;

	auto fmt = ImageFormat::None;
	switch (image.channels()) {
		case 1: fmt = ImageFormat::Lum; break;
		case 3: fmt = ImageFormat::BGR; break;
		case 4: fmt = ImageFormat::BGRX; break;
	}

	if (image.depth() != CV_8U || fmt == ImageFormat::None)
		return {nullptr, 0, 0, ImageFormat::None};

	return {image.data, image.cols, image.rows, fmt};
}

inline ZXing::Results ReadBarcodes(const cv::Mat& image, const ZXing::DecodeHints& hints = {})
{
	ZXing::DecodeHints hints2;
	hints2.setTryHarder(true);
	hints2.setFormats(ZXing::BarcodeFormat::Any);
	//
	return ZXing::ReadBarcodes(ImageViewFromMat(image), hints2);
}

double GetPointDist(cv::Point pt1, cv::Point pt2) {
	return sqrt(std::pow(pt2.x - pt1.x, 2) + std::pow(pt2.y - pt1.y, 2));
}

inline void DrawResult(cv::Mat& img, ZXing::Result res)
{
//	if (res.format() != ZXing::BarcodeFormat::DataMatrix)
//		return;

	auto pos = res.position();
	auto zx2cv = [](ZXing::PointI p) { return cv::Point(p.x, p.y); };
	auto contour = std::vector<cv::Point>{zx2cv(pos[0]), zx2cv(pos[1]), zx2cv(pos[2]), zx2cv(pos[3])};
	const auto* pts = contour.data();
	int npts = contour.size();

#ifndef NO_GUI

	cv::Moments M = cv::moments(contour);
	cv::Point center(M.m10 / M.m00, M.m01 / M.m00);
	cv::circle(img, center, 3, CLR_YELLOW, -1, cv::LINE_AA);
	//
	cv::polylines(img, &pts, &npts, 1, true, CLR_GREEN);
	cv::putText(img, res.text(), zx2cv(pos[3]) + cv::Point(0, 20),
		cv::FONT_HERSHEY_DUPLEX, 0.5, CLR_GREEN);

	if (res.format() == ZXing::BarcodeFormat::DataMatrix) {

		cv::Point p1(center.x, img.rows / 2);	//	img.cols / 2
		cv::line(img, p1, center, CLR_YELLOW, 1, cv::LINE_AA, 0);

		double dist1 = GetPointDist(zx2cv(pos[0]), zx2cv(pos[1]));
		double dist2 = GetPointDist(center, p1) * (DATAMATRIX_W / dist1);

//		cv::putText(img, std::to_string(dist1), zx2cv(pos[3]) + cv::Point(0, 40),
//			cv::FONT_HERSHEY_DUPLEX, 0.5, CLR_GREEN);
		//
		//	Object height on sensor (mm) = (Sensor height (mm) * Object height (pixels)) / Sensor height (pixels)
//		double ohs = (MATRIX_H * dist1) / img.rows;
		//	Distance to Object =  (Real Object height * Focal Length (mm)) / Object height on sensor (mm)
//		double dto = (DATAMATRIX_W * FOCAL_DIST) / ohs;
		//	Real Object height = ((Distance to Object * Object height on sensor (mm)) / Focal Length (mm)
//		double roh = (dto * ohs) / FOCAL_DIST;
		//
//		cv::putText(img, std::to_string(dto), zx2cv(pos[3]) + cv::Point(0, 60),
//				cv::FONT_HERSHEY_DUPLEX, 0.5, CLR_GREEN);
		cv::putText(img, std::to_string(dist2), zx2cv(pos[3]) + cv::Point(0, 40),
						cv::FONT_HERSHEY_DUPLEX, 0.5, CLR_GREEN);
		//
//		cv::line(img, zx2cv(pos[0]), zx2cv(pos[1]), CLR_MAGENTA, 1, cv::LINE_AA, 0);
//		cv::line(img, zx2cv(pos[1]), zx2cv(pos[2]), CLR_CYAN, 1, cv::LINE_AA, 0);

	} else
	//
	if (res.format() == ZXing::BarcodeFormat::Codabar) {

		//

	}

#endif

}

std::string last_bc;
int cnt = 0;

void find_barcodes(cv::Mat& imgColor)
{

	auto results = ReadBarcodes(imgColor);
	for (auto& r : results) {
		DrawResult(imgColor, r);
		if (last_bc != r.text()) {
			last_bc = r.text();
			std::cout << cnt << " Barcode detected = " << last_bc << std::endl;
			cnt++;
		}
	}

}
