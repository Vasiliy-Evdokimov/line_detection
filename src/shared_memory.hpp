/*
 * shared_memory.hpp
 *
 *  Created on: 12 сент. 2023 г.
 *      Author: vevdokimov
 */

#ifndef SHARED_MEMORY_HPP_
#define SHARED_MEMORY_HPP_

#include "config.hpp"

#define SM_NAME 		"/tmp/"
#define CONFIG_SM_ID	19841003
#define CAM1RES_SM_ID	19841004
#define CAM2RES_SM_ID	19841005

#define MAX_POINTS_COUNT	10
#define MAX_HOR_COUNT 		10

struct ResultPointForSM {
	int x;
	int y;
};

struct ParseResultForSM {
	int width;
	int height;
	bool fl_error;
	int points_count;
	ResultPointForSM points[MAX_POINTS_COUNT];
	int hor_count;
	int hor_points[MAX_HOR_COUNT];
};

extern int config_sm_id;
extern ConfigData* config_sm_ptr;

int init_shared_memory();
int read_config_sm(ConfigData& aConfig);
int write_config_sm(ConfigData& aConfig);
int init_config_sm(ConfigData& aConfig);

#endif /* SHARED_MEMORY_HPP_ */
