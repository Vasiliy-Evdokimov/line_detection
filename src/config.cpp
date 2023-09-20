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
ConfigData config_buf;

const char* cfg_filename =
	#ifndef RELEASE
		"/home/vevdokimov/eclipse-workspace/line_detection/Debug/line_detection.cfg";
	#else
		"/home/user/line_detection/line_detection.cfg";
	#endif

std::map<std::string, void*> config_pointers = {
	{ "CAM_ADDR_1",		&config_buf.CAM_ADDR_1 },
	{ "CAM_ADDR_2",		&config_buf.CAM_ADDR_2 },
	{ "UDP_PORT",		&config_buf.UDP_PORT },
	{ "NUM_ROI",		&config_buf.NUM_ROI },
	{ "NUM_ROI_H",		&config_buf.NUM_ROI_H },
	{ "NUM_ROI_V",		&config_buf.NUM_ROI_V },
	{ "SHOW_GRAY",		&config_buf.SHOW_GRAY },
	{ "DRAW_DETAILED",	&config_buf.DRAW_DETAILED },
	{ "DRAW_GRID",		&config_buf.DRAW_GRID },
	{ "DRAW",			&config_buf.DRAW },
	{ "MIN_CONT_LEN",	&config_buf.MIN_CONT_LEN },
	{ "HOR_COLLAPSE",	&config_buf.HOR_COLLAPSE },
	{ "GAUSSIAN_BLUR_KERNEL",	&config_buf.GAUSSIAN_BLUR_KERNEL },
	{ "MORPH_OPEN_KERNEL",		&config_buf.MORPH_OPEN_KERNEL },
	{ "MORPH_CLOSE_KERNEL",		&config_buf.MORPH_CLOSE_KERNEL },
	{ "THRESHOLD_THRESH",		&config_buf.THRESHOLD_THRESH },
	{ "THRESHOLD_MAXVAL",		&config_buf.THRESHOLD_MAXVAL },
	{ "WEB_SHOW_LINES",			&config_buf.WEB_SHOW_LINES },
	{ "WEB_INTERVAL",			&config_buf.WEB_INTERVAL }
};

std::map<std::string, ConfigItem> config_map;

bool restart_threads;
bool kill_threads;

ConfigItem::ConfigItem()
{
	order = 0;
	name = "";
	type = ConfigType::ctUnknown;
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
			int i = aSetting.lookup("value");
			int* val = (int*)config_pointers[name];
			*val = i;
			break;
		}
		case (ConfigType::ctString):
		{
			string s = aSetting.lookup("value");
			char* val = (char*)config_pointers[name];
			strncpy(val, s.c_str(), s.length());
			break;
		}
		case (ConfigType::ctBool):
		{
			int i = aSetting.lookup("value");
			int* val = (int*)config_pointers[name];
			*val = (i > 0) ? 1 : 0;
			break;
		}
	}
}

ConfigItem::ConfigItem(const ConfigItem& src)
{
	order = src.order;
	name = src.name;
	type = src.type;
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

		config_map.clear();
		const Setting &params = root["params"];
		int count = params.getLength();
		for(int i = 0; i < count; i++) {
			ConfigItem new_item(params[i]);
			new_item.order = i + 1;
			config_map.insert({new_item.name, new_item});
		}
		//
		config = config_buf;
		config.recount_data_size();

		config.PID = getpid();
		cout << "Application PID = " << config.PID << endl;

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

	config_buf = aConfig;

	string nm;
	const Setting &params = root["params"];
	int count = params.getLength();
	for(int i = 0; i < count; ++i) {
		Setting& p = params[i];
		p.lookupValue("name", nm);
		if ((config_map.count(nm) == 0) || (config_pointers.count(nm) == 0))
			continue;
		//
		ConfigItem ci = config_map[nm];
		void* ptr = config_pointers[nm];
		//
		switch (ci.type)
		{
			case (ConfigType::ctUnknown):
				/* throw? */
				break;
			case (ConfigType::ctInteger):
			{
				int* val = (int*)ptr;
				p["value"] = *val;
				break;
			}
			case (ConfigType::ctString):
			{
				char* val = (char*)ptr;
				string str(val);
				p["value"] = str;
				break;
			}
			case (ConfigType::ctBool):
			{
				int* val = (int*)ptr;
				p["value"] = ((*val) > 0) ? 1 : 0;
				break;
			}
		}
	}


	try
	{
		cfg.writeFile(cfg_filename);
	}
	catch(const FileIOException &fioex)
	{
		cout << "I/O error while writing file.\n" << fioex.what() << endl;
		throw std::exception();
	}
	catch (const std::exception& e) {
		cout << "An exception occurred:\n" << e.what() << endl;
		throw std::exception();
	}
	catch (...)
	{
		cout << "Another error while writing file.\n";
		throw std::exception();
	}

}

void fill_json_form_config(ConfigData aConfig, Json::Value& js)
{

	config_buf = aConfig;
	string nm;
	char js_nm[255];
	for (const auto& element : config_map) {
		nm = element.first;
		//
		if ((config_map.count(nm) == 0) || (config_pointers.count(nm) == 0))
			continue;
		//
		ConfigItem ci = config_map[nm];
		void* ptr = config_pointers[nm];
		//
		sprintf(js_nm, "%02d_%s", ci.order, nm.c_str());
		//
		switch (ci.type)
		{
			case (ConfigType::ctUnknown):
				/* throw? */
				break;
			case (ConfigType::ctInteger):
			{
				int* val = (int*)ptr;
				js[js_nm] = *val;
				break;
			}
			case (ConfigType::ctString):
			{
				char* val = (char*)ptr;
				js[js_nm] = val;
				break;
			}
			case (ConfigType::ctBool):
			{
				int* val = (int*)ptr;
				js[js_nm] = ((*val) > 0) ? 1 : 0;
				break;
			}
		}
	}

}

void fill_config_form_json(Json::Value js, ConfigData& aConfig)
{

	config_buf = aConfig;
	//
	for (const auto& field : js.getMemberNames()) {
		//	std::cout << "Field: " << field << ", Value: " << js[field] << std::endl;
		//
		if ((config_map.count(field) == 0) || (config_pointers.count(field) == 0))
			continue;
		//
		ConfigItem ci = config_map[field];
		void* ptr = config_pointers[field];
		//
		switch (ci.type)
		{
			case (ConfigType::ctUnknown):
				/* throw? */
				break;
			case (ConfigType::ctInteger):
			{
				int i = stoi(js[field].asString());
				int* val = (int*)ptr;
				*val = i;
				break;
			}
			case (ConfigType::ctString):
			{
				string s = js[field].asString();
				char* val = (char*)ptr;
				strncpy(val, s.c_str(), s.length());
				break;
			}
			case (ConfigType::ctBool):
			{
				int i = stoi(js[field].asString());
				int* val = (int*)ptr;
				*val = (i > 0) ? 1 : 0;
				break;
			}
		}
	}
	//
	aConfig = config_buf;

}

