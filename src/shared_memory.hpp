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
#define CONFIG_SM_ID	19841010
#define CAMRES_SM_ID	19841020
#define DEBUG_SM_ID		19841030

#define MAX_POINTS_COUNT	10
#define MAX_HOR_COUNT 		10

extern int config_sm_id;
extern ConfigData* config_sm_ptr;
extern int results_sm_id[CAM_COUNT];
extern ResultFixed* results_sm_ptr[CAM_COUNT];

int init_shared_memory();

int init_config_sm(ConfigData& aConfig);
int read_config_sm(ConfigData& aConfig);
int write_config_sm(ConfigData& aConfig);

int read_results_sm(ResultFixed& aResult, int aIndex);
int write_results_sm(ResultFixed& aResult, int aIndex);

int read_debug_sm(DebugFixed& aResult, int aIndex);
int write_debug_sm(DebugFixed& aResult, int aIndex);

void parse_result_to_sm(ParseImageResult& parse_result, int aIndex);
void parse_debug_to_sm(DebugFixed& debug_result, int aIndex);

#endif /* SHARED_MEMORY_HPP_ */
