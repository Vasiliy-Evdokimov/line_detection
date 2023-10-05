/*
 * barcode.cpp
 *
 *  Created on: 17 авг. 2023 г.
 *      Author: vevdokimov
 */

#include <string>
#include <cmath>

#include "opencv2/opencv.hpp"

#include "ZXing/ReadBarcode.h"
#include "ZXing/BarcodeFormat.h"

#include "defines.hpp"
#include "config.hpp"
#include "log.hpp"
#include "barcode.hpp"

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

std::string last_bc;
int cnt = 0;

const double D2R = ((2.0 * M_PI) / 360.0);
const double R2D = (180/M_PI);

void pundistors(cv::Point2f &r, const cv::Point2f &a,double w, double h, double dist_fov) // , double dist, double fov
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

void find_barcodes(cv::Mat& img, ParseImageResult& parse_result,
	std::vector<std::vector<cv::Point>>& contours)
{
	auto results = ReadBarcodes(img);

	parse_result.fl_slow_zone = false;
	parse_result.fl_stop_zone = false;
	parse_result.fl_stop_mark = false;
	parse_result.stop_mark_distance = 0;

	for (auto& res : results) {

		std::string txt = res.text();

		auto pos = res.position();
		auto zx2cv = [](ZXing::PointI p) { return cv::Point(p.x, p.y); };
		auto contour = std::vector<cv::Point>{zx2cv(pos[0]), zx2cv(pos[1]), zx2cv(pos[2]), zx2cv(pos[3])};

		contours.push_back(contour);

		cv::Moments M = cv::moments(contour);
		cv::Point2f center(M.m10 / M.m00, M.m01 / M.m00);

#ifndef NO_GUI
		const auto* pts = contour.data();
		int npts = contour.size();

		cv::circle(img, center, 3, CLR_YELLOW, -1, cv::LINE_AA);
		//
		cv::polylines(img, &pts, &npts, 1, true, CLR_GREEN);
		cv::putText(img, res.text(), zx2cv(pos[3]) + cv::Point(0, 20),
			cv::FONT_HERSHEY_DUPLEX, 0.5, CLR_GREEN);
#endif

		if (res.format() == ZXing::BarcodeFormat::DataMatrix) {

			parse_result.fl_stop_mark = true;

			cv::Point2f p1(center.x, img.rows / 2);
			cv::line(img, p1, center, CLR_YELLOW, 1, cv::LINE_AA, 0);

			double dist1 = GetPointDist(zx2cv(pos[1]), zx2cv(pos[2]));
			double dist2 = GetPointDist(center, p1) * (config.DATAMATRIX_WIDTH / dist1);

			cv::Point2f res;
			pundistors(res, center, img.cols, img.rows, -1000);
			cv::putText(img, to_string(res.x) + ";" + to_string(res.y),
				cv::Point2f(50, 50), 1, 1, CLR_GREEN);

//			cv::Point2f pt2;
//			pundistors(p1, pt2);
//			cout << "(" << pt1.x << "; " << pt1.y << ") " << "(" << pt2.x << "; " << pt2.y << ") " << endl;
//			cv::Point cnt(img.cols / 2, img.rows / 2);
//			cv::Point pt12(pt1.x + cnt.x, pt1.y + cnt.y);
//			cv::Point pt22(pt2.x + cnt.x, pt2.y + cnt.y);
//			cv::circle(img, pt12, 3, CLR_GREEN, -1, cv::LINE_AA);
//			cv::circle(img, pt22, 3, CLR_BLUE, -1, cv::LINE_AA);

#ifndef NO_GUI
			cv::putText(img, std::to_string(dist2), zx2cv(pos[3]) + cv::Point(0, 40),
					cv::FONT_HERSHEY_DUPLEX, 0.5, CLR_GREEN);
#endif

			parse_result.stop_mark_distance = round(dist2);

		} else
		//
		if (res.format() == ZXing::BarcodeFormat::Codabar) {

			parse_result.fl_slow_zone |= (txt == "01");
			parse_result.fl_stop_zone |= (txt == "02");

		}

		//	write_log("Barcode detected = " + txt);

	}

}
