/*
 * defines.hpp
 *
 *  Created on: Aug 16, 2023
 *      Author: vevdokimov
 */

#ifndef DEFINES_HPP_
#define DEFINES_HPP_

//#define	SERVICE
//#define	NO_GUI
//#define	RELEASE

#define IMG_WIDTH 	640
#define IMG_HEIGHT 	360

#define CAM_COUNT			2
#define CLEAR_CAM_BUFFER	20

#define MAX_POINTS_COUNT	10
#define MAX_HOR_COUNT		10

//#define USE_UNDISTORT
#define USE_BARCODES
//#define USE_TEMPLATES

//#define UDP_LOG
//#define BARCODE_LOG

#define CLR_BLACK	(cv::Scalar(0x00, 0x00, 0x00))
#define CLR_RED		(cv::Scalar(0x00, 0x00, 0xFF))
#define CLR_BLUE	(cv::Scalar(0xFF, 0x00, 0x00))
#define CLR_GREEN	(cv::Scalar(0x00, 0xFF, 0x00))
#define CLR_YELLOW	(cv::Scalar(0x00, 0xFF, 0xFF))
#define CLR_MAGENTA	(cv::Scalar(0xFF, 0x00, 0xFF))
#define CLR_CYAN	(cv::Scalar(0xFF, 0xFF, 0x00))
#define CLR_WHITE	(cv::Scalar(0xFF, 0xFF, 0xFF))

#define CLR_RECT_BOUND	(cv::Scalar(0xFF, 0x33, 0x33))

#endif /* DEFINES_HPP_ */
