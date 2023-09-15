/*
 * shared_memory.hpp
 *
 *  Created on: 12 сент. 2023 г.
 *      Author: vevdokimov
 */

#ifndef SHARED_MEMORY_HPP_
#define SHARED_MEMORY_HPP_

#include "common_types.hpp"
#include "config.hpp"

#define SM_NAME 		"/tmp/"
#define CONFIG_SM_ID	19841003
#define CAMRES_SM_ID	19841004

#define MAX_POINTS_COUNT	10
#define MAX_HOR_COUNT 		10

extern int config_sm_id;
extern ConfigData* config_sm_ptr;
extern int results_sm_id[CAM_COUNT];
extern ResultFixed* results_sm_ptr[CAM_COUNT];

int init_shared_memory();
int read_config_sm(ConfigData& aConfig);
int write_config_sm(ConfigData& aConfig);
int init_config_sm(ConfigData& aConfig);
int write_results_sm(ResultFixed& aResult, int aIndex);
int read_results_sm(ResultFixed& aResult, int aIndex);

#endif /* SHARED_MEMORY_HPP_ */
