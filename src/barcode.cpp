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

#include "config.hpp"
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

inline ZXing::Results ReadBarcodes(const cv::Mat& image, const bool slow_stop_found)
{
	ZXing::DecodeHints hints2;
	//
	hints2.setTryHarder(config.BARCODE_TRY_HARDER);
	hints2.setTryInvert(config.BARCODE_TRY_INVERT);
	hints2.setTryRotate(config.BARCODE_TRY_ROTATE);
	//
	if (config.DATAMATRIX_SEARCH || (!config.DATAMATRIX_SEARCH && slow_stop_found))
		hints2.setFormats(ZXing::BarcodeFormat::Codabar | ZXing::BarcodeFormat::DataMatrix);
	else
		hints2.setFormats(ZXing::BarcodeFormat::Codabar);
	//
	return ZXing::ReadBarcodes(ImageViewFromMat(image), hints2);
}

double get_points_distance(cv::Point pt1, cv::Point pt2)
{
	return sqrt(std::pow(pt2.x - pt1.x, 2) + std::pow(pt2.y - pt1.y, 2));
}

void barcodes_detect(cv::Mat& img,
	std::vector<BarcodeDetectionResult>& detection_results,
	const bool slow_stop_found)
{
	auto results = ReadBarcodes(img, slow_stop_found);

	for (auto& res : results)
	{
		BarcodeDetectionResult new_result;

		std::string txt = res.text();
		new_result.text = res.text();

		auto pos = res.position();
		auto zx2cv = [](ZXing::PointI p) { return cv::Point(p.x, p.y); };
		auto contour = std::vector<cv::Point>{zx2cv(pos[0]), zx2cv(pos[1]), zx2cv(pos[2]), zx2cv(pos[3])};
		new_result.contour = contour;

		if (res.format() == ZXing::BarcodeFormat::DataMatrix)
		{
			if (txt == "STOP") new_result.barcode_type = 3;
		}
		//
		else if (res.format() == ZXing::BarcodeFormat::Codabar)
		{
			if (txt == "01") new_result.barcode_type = 1;
			if (txt == "02") new_result.barcode_type = 2;
		}

		if (new_result.barcode_type)
			detection_results.push_back(new_result);
	}
}
