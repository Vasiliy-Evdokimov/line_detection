/*
 * barcode.hpp
 *
 *  Created on: 17 авг. 2023 г.
 *      Author: vevdokimov
 */

#ifndef BARCODE_HPP_
#define BARCODE_HPP_

#include "opencv2/core/types.hpp"
#include "common_types.hpp"

void find_barcodes(cv::Mat& img, ParseImageResult& parse_result);

#endif /* BARCODE_HPP_ */
