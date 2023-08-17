/*
 * udp.hpp
 *
 *  Created on: Aug 16, 2023
 *      Author: vevdokimov
 */

#ifndef UDP_HPP_
#define UDP_HPP_

#include <mutex>

using namespace std;

struct udp_point {
	uint16_t x;
	uint16_t y;
};

struct udp_package {
	uint16_t counter;
	uint16_t img_width;
	uint16_t img_height;
	uint8_t error_flag;
	uint8_t points_count;
	udp_point points[4];
	uint16_t points_hor[4];
};

extern mutex udp_packs_mtx;
extern udp_package udp_packs[2];

void udp_func();

#endif /* UDP_HPP_ */
