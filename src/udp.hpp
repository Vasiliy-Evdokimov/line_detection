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
const int UDP_RESULT_SIZE = (
	sizeof(ResultFixed)
	- sizeof(high_resolution_clock::time_point)	//	время измерения
	- sizeof(int16_t)	//	флаги пульта
	- sizeof(int16_t)	//	высота подъема гидравлики
	);

struct UdpPackage {
	uint16_t counter;
	uint8_t results[UDP_RESULT_SIZE * CAM_COUNT];
	uint16_t crc;
};

struct UdpRequest {
	char request[16];	//	'c', 'a', 'm', 'e', 'r', a'
	int16_t pult_flags;
	int16_t hidro_height[CAM_COUNT];
};

extern pthread_t udp_thread_id;

extern UdpRequest udp_request;

void kill_udp_thread();

void udp_func();

#endif /* UDP_HPP_ */
