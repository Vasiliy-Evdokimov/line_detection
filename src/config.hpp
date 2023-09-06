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
	string CAM_ADDR_1;
	string CAM_ADDR_2;
	//
	char UDP_ADDR[15];
	int UDP_PORT;
	//
	int NUM_ROI;
	int NUM_ROI_H;
	int NUM_ROI_V;
	int DATA_SIZE;
	//
	int SHOW_GRAY;
	int DETAILED;
	int DRAW_GRID;
	int DRAW;
	//
	int MIN_CONT_LEN;
	int HOR_COLLAPSE;
	//
	int GAUSSIAN_BLUR_KERNEL;
	int MORPH_OPEN_KERNEL;
	int MORPH_CLOSE_KERNEL;
	//
	int THRESHOLD_THRESH;
	int THRESHOLD_MAXVAL;
};

extern ConfigData config;

extern bool restart_threads;
extern bool kill_threads;

void recount_data_size(ConfigData& cfg);

void read_config(char* exe);

#endif /* CONFIG_HPP_ */
