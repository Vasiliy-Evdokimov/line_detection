/*
 * config.hpp
 *
 *  Created on: Aug 17, 2023
 *      Author: vevdokimov
 */

#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <string.h>

#include <libconfig.h++>
#include <json/json.h>

using namespace std;
using namespace libconfig;

enum class ConfigType { ctUnknown = 0, ctInteger, ctString, ctBool };

struct ConfigItem
{
	int order;
	string name;
	ConfigType type;
	string description;

	ConfigItem();
	ConfigItem(Setting& aSetting);
	ConfigItem(const ConfigItem& src);
};

struct ConfigData
{
	int PID;
	//
	char CAM_ADDR_1[255];
	char CAM_ADDR_2[255];
	//
	int USE_CAM;
	int USE_FPS;
	int FPS;
	//
	int UDP_PORT;
	char UDP_REQUEST[50];
	//
	int NUM_ROI;
	//
	int MIN_CONT_LEN;
	int MAX_CONT_LEN;
	int MIN_RECT_WIDTH;
	int MAX_RECT_WIDTH;
	//
	int GAUSSIAN_BLUR_KERNEL;
	int MORPH_OPEN_KERNEL;
	int MORPH_CLOSE_KERNEL;
	//
	int THRESHOLD_THRESH;
	int THRESHOLD_MAXVAL;
	int THRESHOLD_HEIGHT_K;
	//
	int FILTER_NEIGHBOR;
	//
	int AUTO_EMULATE;
	int AUTO_ONE_POINT;
	//
	int WEB_SHOW_LINES;
	int WEB_INTERVAL;
	//
	int BARCODE_TRY_HARDER;
	int BARCODE_TRY_INVERT;
	int BARCODE_TRY_ROTATE;
	//
	int BARCODE_LEFT;
	int DATAMATRIX_WIDTH;
	int DATAMATRIX_SEARCH;
	//
	int CAM_TIMEOUT;
	//
	int STATS_LOG;
	int STATS_COUNT;
	//
	int WEB_DEBUG;
};

extern ConfigData config;
extern std::map<std::string, ConfigItem> config_map;

extern bool restart_threads;
extern bool kill_threads;

void read_config();
void save_config(ConfigData aConfig);

void fill_json_form_config(ConfigData aConfig, Json::Value& js);
void fill_config_form_json(Json::Value js, ConfigData& aConfig);

#endif /* CONFIG_HPP_ */
