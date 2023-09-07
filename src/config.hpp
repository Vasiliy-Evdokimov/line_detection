/*
 * config.hpp
 *
 *  Created on: Aug 17, 2023
 *      Author: vevdokimov
 */

#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <string>
#include <libconfig.h++>

using namespace std;
using namespace libconfig;

enum class ConfigType { ctUnknown = 0, ctInteger, ctString, ctBool };

struct ConfigItem
{
	string name;
	ConfigType type;
	int valueInt;
	string valueString;
	bool valueBool;
	string description;

	ConfigItem();
	ConfigItem(Setting& aSetting);
	ConfigItem(const ConfigItem& src);
};

struct ConfigData
{
	ConfigItem items[100];
	int items_count;

	int GetIntParam(string aName);
	string GetStringParam(string aName);
	bool GetBoolParam(string aName);
	void recount_data_size();

	string CAM_ADDR_1;
	string CAM_ADDR_2;
	//
	string UDP_ADDR;
	int UDP_PORT;
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
