/*
 * config.cpp
 *
 *  Created on: Aug 17, 2023
 *      Author: vevdokimov
 */

#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <libconfig.h++>

#include "defines.hpp"
#include "config_path.hpp"
#include "log.hpp"
#include "config.hpp"

using namespace std;
using namespace libconfig;

ConfigData config;
ConfigData config_buf;

const string config_filename = "line_detection.cfg";

std::map<std::string, void*> config_pointers = {
	{ "CAM_ADDR_1",		&config_buf.CAM_ADDR_1 },
	{ "CAM_ADDR_2",		&config_buf.CAM_ADDR_2 },
	{ "USE_CAM",		&config_buf.USE_CAM },
	{ "USE_FPS",		&config_buf.USE_FPS },
	{ "FPS",			&config_buf.FPS },
	{ "UDP_PORT",		&config_buf.UDP_PORT },
	{ "UDP_REQUEST",	&config_buf.UDP_REQUEST },
	{ "NUM_ROI",		&config_buf.NUM_ROI },
	{ "MIN_CONT_LEN",	&config_buf.MIN_CONT_LEN },
	{ "MAX_CONT_LEN",	&config_buf.MAX_CONT_LEN },
	{ "MIN_RECT_WIDTH",	&config_buf.MIN_RECT_WIDTH },
	{ "MAX_RECT_WIDTH",	&config_buf.MAX_RECT_WIDTH },
	{ "GAUSSIAN_BLUR_KERNEL",	&config_buf.GAUSSIAN_BLUR_KERNEL },
	{ "MORPH_OPEN_KERNEL",		&config_buf.MORPH_OPEN_KERNEL },
	{ "MORPH_CLOSE_KERNEL",		&config_buf.MORPH_CLOSE_KERNEL },
	{ "THRESHOLD_THRESH",		&config_buf.THRESHOLD_THRESH },
	{ "THRESHOLD_MAXVAL",		&config_buf.THRESHOLD_MAXVAL },
	{ "THRESHOLD_HEIGHT_K",		&config_buf.THRESHOLD_HEIGHT_K },
	{ "FILTER_NEIGHBOR",		&config_buf.FILTER_NEIGHBOR },
	{ "WEB_SHOW_LINES",			&config_buf.WEB_SHOW_LINES },
	{ "WEB_INTERVAL",			&config_buf.WEB_INTERVAL },
	{ "BARCODE_TRY_HARDER",		&config_buf.BARCODE_TRY_HARDER },
	{ "BARCODE_TRY_INVERT",		&config_buf.BARCODE_TRY_INVERT },
	{ "BARCODE_TRY_ROTATE",		&config_buf.BARCODE_TRY_ROTATE },
	{ "BARCODE_LEFT",			&config_buf.BARCODE_LEFT },
	{ "DATAMATRIX_WIDTH",		&config_buf.DATAMATRIX_WIDTH },
	{ "DATAMATRIX_SEARCH",		&config_buf.DATAMATRIX_SEARCH },
	{ "CAM_TIMEOUT",	&config_buf.CAM_TIMEOUT },
	{ "STATS_LOG",		&config_buf.STATS_LOG },
	{ "STATS_COUNT",	&config_buf.STATS_COUNT },
	{ "WEB_DEBUG",		&config_buf.WEB_DEBUG }
};

std::map<std::string, ConfigItem> config_map;

bool restart_threads = false;
bool kill_threads = false;

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

void read_config()
{
	string config_file_path =
		get_config_directory() + config_filename;
	write_log("config_file_path = " + config_file_path);

	Config cfg;

	try
	{
		cfg.readFile(config_file_path.c_str());
	}
	catch(const FileIOException &fioex)
	{
		write_err("I/O error while reading file: " + config_file_path);
		exit(1);
	}
	catch(const ParseException &pex)
	{
		string str_file(pex.getFile());
		write_err("Parse error at " + str_file + ":" + to_string(pex.getLine()) + " - " + pex.getError());
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

		config.PID = getpid();
		write_log("Application PID = " + to_string(config.PID));

		restart_threads = false;

	}
	catch(const SettingNotFoundException &nfex)
	{
		write_err("Critical setting was not found in configuration file.");
		exit(1);
	}

}

void save_config(ConfigData aConfig)
{
	string config_file_path =
		get_config_directory() + config_filename;

	Config cfg;

	try
	{
		cfg.readFile(config_file_path.c_str());
	}
	catch(const FileIOException &fioex)
	{
		write_err("I/O error while reading file: " + config_file_path);
		exit(1);
	}
	catch(const ParseException &pex)
	{
		string str_file(pex.getFile());
		write_err("Parse error at " + str_file + ":" + to_string(pex.getLine()) + " - " + pex.getError());
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
		cfg.writeFile(config_file_path.c_str());
	}
	catch(const FileIOException &fioex)
	{
		string str(fioex.what());
		write_log("I/O error while writing file: " + config_file_path);
		throw std::exception();
	}
	catch (const std::exception& e) {
		string str(e.what());
		write_log("An exception occurred: " + str);
		throw std::exception();
	}
	catch (...)
	{
		write_log("Another error while writing file.");
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
				string buf(val);
				memset(val, 0, buf.length());
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

