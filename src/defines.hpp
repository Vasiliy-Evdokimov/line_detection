/*
 * defines.hpp
 *
 *  Created on: Aug 16, 2023
 *      Author: vevdokimov
 */

#ifndef DEFINES_HPP_
#define DEFINES_HPP_

//#define	NO_GUI
//#define	RELEASE

#define CAM_COUNT	2

#define THREAD_NAMING	1

#define DIR_LEFT	-1
#define DIR_RIGHT	1
#define DIR_FORWARD	0

#define CLR_BLACK	(cv::Scalar(0x00, 0x00, 0x00))
#define CLR_RED		(cv::Scalar(0x00, 0x00, 0xFF))
#define CLR_BLUE	(cv::Scalar(0xFF, 0x00, 0x00))
#define CLR_GREEN	(cv::Scalar(0x00, 0xFF, 0x00))
#define CLR_YELLOW	(cv::Scalar(0x00, 0xFF, 0xFF))
#define CLR_MAGENTA	(cv::Scalar(0xFF, 0x00, 0xFF))
#define CLR_CYAN	(cv::Scalar(0xFF, 0xFF, 0x00))

#define CLR_RECT_BOUND	(cv::Scalar(0xFF, 0x33, 0x33))

#define CLEAR_CAM_BUFFER	20

#define AVG_CNT		100
#define STATS_LOG	1
#define UDP_LOG		0

#define MAX_POINTS_COUNT	10
#define MAX_HOR_COUNT		10

#endif /* DEFINES_HPP_ */
