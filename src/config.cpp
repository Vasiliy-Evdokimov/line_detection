/*
 * config.cpp
 *
 *  Created on: Aug 17, 2023
 *      Author: vevdokimov
 */

#include <iostream>
#include <string.h>
#include <libconfig.h++>

#include "config.hpp"

using namespace std;
using namespace libconfig;

ConfigData config;

char cfg_filename[255];

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

int ConfigData::GetIntParam(string aName)
{
	return 0;
}

string ConfigData::GetStringParam(string aName)
{
	return "";
}

bool ConfigData::GetBoolParam(string aName)
{
	return false;
}

void read_config(char* exe)
{

	sprintf(cfg_filename, "%s.cfg", exe);

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
		root["cameras_addresses"].lookupValue("address_1", config.CAM_ADDR_1);
		root["cameras_addresses"].lookupValue("address_2", config.CAM_ADDR_2);

		root["udp_parameters"].lookupValue("address", config.UDP_ADDR);
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

		restart_threads = false;
		kill_threads = false;

	}
	catch(const SettingNotFoundException &nfex)
	{
		cerr << "Critical setting was not found in configuration file.\n";
		exit(1);
	}

}

void save_config()
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

	root["cameras_addresses"]["address_1"] = config.CAM_ADDR_1;
	root["cameras_addresses"]["address_2"] = config.CAM_ADDR_2;

	root["udp_parameters"]["address"] = config.UDP_ADDR;
	root["udp_parameters"]["port"] = config.UDP_PORT;

	root["regions_of_interests"]["roi"] = config.NUM_ROI;
	root["regions_of_interests"]["roi_h"] = config.NUM_ROI_H;
	root["regions_of_interests"]["roi_v"] = config.NUM_ROI_V;
	config.recount_data_size();

	root["processing"]["gaussian_blur"]["kernel"] = config.GAUSSIAN_BLUR_KERNEL;
	root["processing"]["morph_open"]["kernel"] = config.MORPH_OPEN_KERNEL;
	root["processing"]["morph_close"]["kernel"] = config.MORPH_CLOSE_KERNEL;
	root["processing"]["threshold"]["thresh"] = config.THRESHOLD_THRESH;
	root["processing"]["threshold"]["maxval"] = config.THRESHOLD_MAXVAL;

	root["parsing"]["minimal_contour_length"] = config.MIN_CONT_LEN;
	root["parsing"]["horizontal_collapse"] = config.HOR_COLLAPSE;

	root["displaying"]["show_gray"] = config.SHOW_GRAY;
	root["displaying"]["draw_detailed"] = config.DRAW_DETAILED;
	root["displaying"]["draw_grid"] = config.DRAW_GRID;
	root["displaying"]["draw"] = config.DRAW;

	cfg.writeFile(cfg_filename);

}
