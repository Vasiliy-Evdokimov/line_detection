/*
 * config.cpp
 *
 *  Created on: Aug 17, 2023
 *      Author: vevdokimov
 */

#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <libconfig.h++>

#include "defines.hpp"
#include "config.hpp"

using namespace std;
using namespace libconfig;

ConfigData config;

const char* cfg_filename =
	#ifndef RELEASE
		"/home/vevdokimov/eclipse-workspace/line_detection/Debug/line_detection.cfg";
	#else
		"/home/user/line_detection/line_detection.cfg";
	#endif

bool restart_threads;
bool kill_threads;

ConfigItem::ConfigItem()
{
	name = "";
	type = ConfigType::ctUnknown;
	valueInt = 0;
	valueString = "";
	valueBool = false;
	description = "";
}

ConfigItem::ConfigItem(Setting& aSetting) : ConfigItem()
{
	aSetting.lookupValue("name", name);
	int lookup_type = aSetting.lookup("type");
	type = (ConfigType)lookup_type;
	aSetting.lookupValue("description", description);
	//
	switch (type)
	{
		case (ConfigType::ctUnknown):
			/* throw? */
			break;
		case (ConfigType::ctInteger):
		{
			valueInt = aSetting.lookup("value");
			break;
		}
		case (ConfigType::ctString):
		{
			string s = aSetting.lookup("value");
			valueString = s;
			break;
		}
		case (ConfigType::ctBool):
		{
			int i = aSetting.lookup("value");
			valueBool = (bool)i;
			break;
		}
	}
}

ConfigItem::ConfigItem(const ConfigItem& src)
{
	name = src.name;
	type = src.type;
	valueInt = src.valueInt;
	valueString = src.valueString;
	valueBool = src.valueBool;
	description = src.description;
}

void ConfigData::recount_data_size() {
	DATA_SIZE =	(NUM_ROI_H * NUM_ROI_V + (NUM_ROI - NUM_ROI_H));
}

void read_config()
{

	cout << "cfg_filename = " << cfg_filename << endl;

	Config cfg;

	try
	{
		cfg.readFile(cfg_filename);
	}
	catch(const FileIOException &fioex)
	{
		cerr << "I/O error while reading file.\n";
		exit(1);
	}
	catch(const ParseException &pex)
	{
		cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
				<< " - " << pex.getError() << endl;
		exit(1);
	}

	Setting& root = cfg.getRoot();

	try {
		/*
		const Setting &params = root["params"];
		int count = params.getLength();
		config.items_count = 0;

		for(int i = 0; i < count; ++i) {
			ConfigItem new_item(params[i]);
			config.items[i] = new_item;
			config.items_count++;
		}
		*/
		config.PID = getpid();
		cout << "Application PID = " << config.PID << endl;

		string str;
		root["cameras_addresses"].lookupValue("address_1", str);
		strcpy(config.CAM_ADDR_1, str.c_str());
		root["cameras_addresses"].lookupValue("address_2", str);
		strcpy(config.CAM_ADDR_2, str.c_str());

		root["udp_parameters"].lookupValue("port", config.UDP_PORT);

		root["regions_of_interests"].lookupValue("roi", config.NUM_ROI);
		root["regions_of_interests"].lookupValue("roi_h", config.NUM_ROI_H);
		root["regions_of_interests"].lookupValue("roi_v", config.NUM_ROI_V);
		config.recount_data_size();

		root["processing"]["gaussian_blur"].lookupValue("kernel", config.GAUSSIAN_BLUR_KERNEL);
		root["processing"]["morph_open"].lookupValue("kernel", config.MORPH_OPEN_KERNEL);
		root["processing"]["morph_close"].lookupValue("kernel", config.MORPH_CLOSE_KERNEL);
		root["processing"]["threshold"].lookupValue("thresh", config.THRESHOLD_THRESH);
		root["processing"]["threshold"].lookupValue("maxval", config.THRESHOLD_MAXVAL);

		root["parsing"].lookupValue("minimal_contour_length", config.MIN_CONT_LEN);
		root["parsing"].lookupValue("horizontal_collapse", config.HOR_COLLAPSE);

		root["displaying"].lookupValue("show_gray", config.SHOW_GRAY);
		root["displaying"].lookupValue("draw_detailed", config.DRAW_DETAILED);
		root["displaying"].lookupValue("draw_grid", config.DRAW_GRID);
		root["displaying"].lookupValue("draw", config.DRAW);

		root["web_interface"].lookupValue("show_lines", config.WEB_SHOW_LINES);
		root["web_interface"].lookupValue("interval", config.WEB_INTERVAL);

		restart_threads = false;

	}
	catch(const SettingNotFoundException &nfex)
	{
		cerr << "Critical setting was not found in configuration file.\n";
		exit(1);
	}

}

void save_config(ConfigData aConfig)
{

	Config cfg;

	try
	{
		cfg.readFile(cfg_filename);
	}
	catch(const FileIOException &fioex)
	{
		cerr << "I/O error while reading file.\n";
		exit(1);
	}
	catch(const ParseException &pex)
	{
		cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
				<< " - " << pex.getError() << endl;
		exit(1);
	}

	Setting& root = cfg.getRoot();

	root["cameras_addresses"]["address_1"] = aConfig.CAM_ADDR_1;
	root["cameras_addresses"]["address_2"] = aConfig.CAM_ADDR_2;

	root["udp_parameters"]["port"] = aConfig.UDP_PORT;

	root["regions_of_interests"]["roi"] = aConfig.NUM_ROI;
	root["regions_of_interests"]["roi_h"] = aConfig.NUM_ROI_H;
	root["regions_of_interests"]["roi_v"] = aConfig.NUM_ROI_V;
	aConfig.recount_data_size();

	root["processing"]["gaussian_blur"]["kernel"] = aConfig.GAUSSIAN_BLUR_KERNEL;
	root["processing"]["morph_open"]["kernel"] = aConfig.MORPH_OPEN_KERNEL;
	root["processing"]["morph_close"]["kernel"] = aConfig.MORPH_CLOSE_KERNEL;
	root["processing"]["threshold"]["thresh"] = aConfig.THRESHOLD_THRESH;
	root["processing"]["threshold"]["maxval"] = aConfig.THRESHOLD_MAXVAL;

	root["parsing"]["minimal_contour_length"] = aConfig.MIN_CONT_LEN;
	root["parsing"]["horizontal_collapse"] = aConfig.HOR_COLLAPSE;

	root["displaying"]["show_gray"] = aConfig.SHOW_GRAY;
	root["displaying"]["draw_detailed"] = aConfig.DRAW_DETAILED;
	root["displaying"]["draw_grid"] = aConfig.DRAW_GRID;
	root["displaying"]["draw"] = aConfig.DRAW;

	root["web_interface"]["show_lines"] = config.WEB_SHOW_LINES;
	root["web_interface"]["interval"] = config.WEB_INTERVAL;

	cfg.writeFile(cfg_filename);

}

void fill_json_form_config(ConfigData aConfig, Json::Value& js)
{

	js["01_CAM_ADDR_1"] = aConfig.CAM_ADDR_1;
	js["02_CAM_ADDR_2"] = aConfig.CAM_ADDR_2;
	//
	js["04_UDP_PORT"] = aConfig.UDP_PORT;
	//
	js["05_NUM_ROI"] = aConfig.NUM_ROI;
	js["06_NUM_ROI_H"] = aConfig.NUM_ROI_H;
	js["07_NUM_ROI_V"] = aConfig.NUM_ROI_V;
	//
	js["08_SHOW_GRAY"] = aConfig.SHOW_GRAY;
	js["09_DRAW_DETAILED"] = aConfig.DRAW_DETAILED;
	js["10_DRAW_GRID"] = aConfig.DRAW_GRID;
	js["11_DRAW"] = aConfig.DRAW;
	//
	js["12_MIN_CONT_LEN"] = aConfig.MIN_CONT_LEN;
	js["13_HOR_COLLAPSE"] = aConfig.HOR_COLLAPSE;
	//
	js["14_GAUSSIAN_BLUR_KERNEL"] = aConfig.GAUSSIAN_BLUR_KERNEL;
	js["15_MORPH_OPEN_KERNEL"] = aConfig.MORPH_OPEN_KERNEL;
	js["16_MORPH_CLOSE_KERNEL"] = aConfig.MORPH_CLOSE_KERNEL;
	//
	js["17_THRESHOLD_THRESH"] = aConfig.THRESHOLD_THRESH;
	js["18_THRESHOLD_MAXVAL"] = aConfig.THRESHOLD_MAXVAL;
	//
	js["19_WEB_SHOW_LINES"] = aConfig.WEB_SHOW_LINES;
	js["20_WEB_INTERVAL"] = aConfig.WEB_INTERVAL;

}

void fill_config_form_json(Json::Value js, ConfigData& aConfig)
{
	string str;
	//
	str = js["CAM_ADDR_1"].asString();
	strcpy(aConfig.CAM_ADDR_1, str.c_str());
	str = js["CAM_ADDR_2"].asString();
	strcpy(aConfig.CAM_ADDR_2, str.c_str());
	//
	aConfig.UDP_PORT = stoi(js["UDP_PORT"].asString());
	//
	aConfig.NUM_ROI = stoi(js["NUM_ROI"].asString());
	aConfig.NUM_ROI_H = stoi(js["NUM_ROI_H"].asString());
	aConfig.NUM_ROI_V = stoi(js["NUM_ROI_V"].asString());
	aConfig.recount_data_size();
	//
	aConfig.SHOW_GRAY = stoi(js["SHOW_GRAY"].asString());
	aConfig.DRAW_DETAILED = stoi(js["DRAW_DETAILED"].asString());
	aConfig.DRAW_GRID = stoi(js["DRAW_GRID"].asString());
	aConfig.DRAW = stoi(js["DRAW"].asString());
	//
	aConfig.MIN_CONT_LEN = stoi(js["MIN_CONT_LEN"].asString());
	aConfig.HOR_COLLAPSE = stoi(js["HOR_COLLAPSE"].asString());
	//
	aConfig.GAUSSIAN_BLUR_KERNEL = stoi(js["GAUSSIAN_BLUR_KERNEL"].asString());
	aConfig.MORPH_OPEN_KERNEL = stoi(js["MORPH_OPEN_KERNEL"].asString());
	aConfig.MORPH_CLOSE_KERNEL = stoi(js["MORPH_CLOSE_KERNEL"].asString());
	//
	aConfig.THRESHOLD_THRESH = stoi(js["THRESHOLD_THRESH"].asString());
	aConfig.THRESHOLD_MAXVAL = stoi(js["THRESHOLD_MAXVAL"].asString());
	//
	aConfig.WEB_SHOW_LINES = stoi(js["WEB_SHOW_LINES"].asString());
	aConfig.WEB_INTERVAL = stoi(js["WEB_INTERVAL"].asString());
}

