/*
 * barcode.hpp
 *
 *  Created on: 17 авг. 2023 г.
 *      Author: vevdokimov
 */

#ifndef BARCODE_HPP_
#define BARCODE_HPP_

#include "opencv2/core/types.hpp"

struct BarcodeDetectionResult
{
	int barcode_type;
	std::vector<cv::Point> contour;
	std::string text;
};

double get_points_distance(cv::Point pt1, cv::Point pt2);

void barcodes_detect(cv::Mat& img,
	std::vector<BarcodeDetectionResult>& detection_results,
	const bool slow_stop_found);

#endif /* BARCODE_HPP_ */
