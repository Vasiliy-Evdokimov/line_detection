/*
 * config.hpp
 *
 *  Created on: Aug 17, 2023
 *      Author: vevdokimov
 */

#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <opencv2/core.hpp>
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
	int USE_CAM;
	//
	int UDP_PORT;
	char UDP_REQUEST[50];
	//
	int NUM_ROI;
	int NUM_ROI_H;
	int NUM_ROI_V;
	int DATA_SIZE;
	//
	int SHOW_GRAY;
	int DRAW_DETAILED;
	int DRAW_GRID;
	int DRAW;
	//
	int MIN_CONT_LEN;
	int MIN_RECT_WIDTH;
	int HOR_COLLAPSE;
	//
	int GAUSSIAN_BLUR_KERNEL;
	int MORPH_OPEN_KERNEL;
	int MORPH_CLOSE_KERNEL;
	//
	int THRESHOLD_THRESH;
	int THRESHOLD_MAXVAL;
	//
	int WEB_SHOW_LINES;
	int WEB_INTERVAL;
	//
	int BARCODE_LEFT;
	int BARCODE_WIDTH;
	int DATAMATRIX_WIDTH;
	//
	int LINE_WIDTH;
	//
	int CAM_TIMEOUT;
	//
	int CALIBRATE_CAM;

	void recount_data_size();
};

extern ConfigData config;
extern std::map<std::string, ConfigItem> config_map;

extern bool restart_threads;
extern bool kill_threads;

extern cv::Mat cameraMatrix;
extern cv::Mat distCoeffs;

void read_calibration();

void read_config();
void save_config(ConfigData aConfig);

void fill_json_form_config(ConfigData aConfig, Json::Value& js);
void fill_config_form_json(Json::Value js, ConfigData& aConfig);

#endif /* CONFIG_HPP_ */
