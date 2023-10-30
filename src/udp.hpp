/*
 * udp.hpp
 *
 *  Created on: Aug 16, 2023
 *      Author: vevdokimov
 */

#ifndef UDP_HPP_
#define UDP_HPP_

#include "defines.hpp"
#include "common_types.hpp"

using namespace std;

//	не отправляем по UDP время результата
const int UDP_RESULT_SIZE = (sizeof(ResultFixed) - sizeof(high_resolution_clock::time_point));

struct udp_package {
	uint16_t counter;
	uint8_t results[UDP_RESULT_SIZE * 2];
	uint16_t crc;
};

extern pthread_t udp_thread_id;

void kill_udp_thread();

void udp_func();

#endif /* UDP_HPP_ */
