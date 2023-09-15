/*
 * udp.hpp
 *
 *  Created on: Aug 16, 2023
 *      Author: vevdokimov
 */

#ifndef UDP_HPP_
#define UDP_HPP_

#include "common_types.hpp"

using namespace std;

struct udp_package {
	uint16_t counter;
	ResultFixed results[2];
};

extern pthread_t udp_thread_id;

void kill_udp_thread();

void udp_func();

#endif /* UDP_HPP_ */
