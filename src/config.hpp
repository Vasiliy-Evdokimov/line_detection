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
	string cam_addr_1;
	string cam_addr_2;
	char UDP_ADDR[15];
	int UDP_PORT;
};

extern ConfigData config;

void read_config(char* exe);

#endif /* CONFIG_HPP_ */
