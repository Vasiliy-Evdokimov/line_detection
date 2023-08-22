/*
 * defines.hpp
 *
 *  Created on: Aug 16, 2023
 *      Author: vevdokimov
 */

#ifndef DEFINES_HPP_
#define DEFINES_HPP_

#define UDP_BUFLEN 255

#define NUM_ROI		4	//	общее количество "строк"
#define NUM_ROI_H	4	//	количество "строк" снизу, которые делим на "столбцы"
#define NUM_ROI_V	3	//	количество "столбцов" в "строке"
#define DATA_SIZE (NUM_ROI_H * NUM_ROI_V + (NUM_ROI - NUM_ROI_H))

#define HOR_ANGLE		10
#define VER_ANGLE		5
#define MIN_CONT_LEN	(100.0)
#define HOR_COLLAPSE	100

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

#define SHOW_CAM	0b01
#define DETAILED	0
#define SHOW_GRAY	0
#define DRAW_GRID	0
#define DRAW		1
#define MORPHOLOGY	1

#define AVG_CNT		1
#define STATS_LOG	0
#define UDP_LOG		0

#endif /* DEFINES_HPP_ */
