/*
 * config.hpp
 *
 *  Created on: Aug 17, 2023
 *      Author: vevdokimov
 */

#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <string>

using namespace std;

struct ConfigData
{
	int roi_amount;
	int roi_h_amount;
	int roi_v_amount;
	string cam_addr_1;
	string cam_addr_2;
	char udp_addr[15];
	int udp_port;
};

extern ConfigData config;

void read_config(char* exe);

#endif /* CONFIG_HPP_ */
