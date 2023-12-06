/*
 * config.cpp
 *
 *  Created on: Aug 17, 2023
 *      Author: vevdokimov
 */

#include <opencv2/core.hpp>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <libconfig.h++>

#include "defines.hpp"
#include "config.hpp"
#include "log.hpp"

using namespace std;
using namespace libconfig;

ConfigData config;
ConfigData config_buf;

cv::Mat cameraMatrix, distCoeffs;

const string app_folder =
	#ifndef RELEASE
		"/home/vevdokimov/eclipse-workspace/line_detection/Debug/";
	#else
		"/home/user/line_detection/";
	#endif

const string config_filename = app_folder + "line_detection.cfg";
const string calibration_filename = app_folder + "calibration.xml";

std::map<std::string, void*> config_pointers = {
	{ "CAM_ADDR_1",		&config_buf.CAM_ADDR_1 },
	{ "CAM_ADDR_2",		&config_buf.CAM_ADDR_2 },
	{ "USE_CAM",		&config_buf.USE_CAM },
	{ "UDP_PORT",		&config_buf.UDP_PORT },
	{ "UDP_REQUEST",	&config_buf.UDP_REQUEST },
	{ "NUM_ROI",		&config_buf.NUM_ROI },
	{ "NUM_ROI_H",		&config_buf.NUM_ROI_H },
	{ "NUM_ROI_V",		&config_buf.NUM_ROI_V },
	{ "SHOW_GRAY",		&config_buf.SHOW_GRAY },
	{ "DRAW_DETAILED",	&config_buf.DRAW_DETAILED },
	{ "DRAW_GRID",		&config_buf.DRAW_GRID },
	{ "DRAW",			&config_buf.DRAW },
	{ "MIN_CONT_LEN",	&config_buf.MIN_CONT_LEN },
	{ "MIN_RECT_WIDTH",	&config_buf.MIN_RECT_WIDTH },
	{ "HOR_COLLAPSE",	&config_buf.HOR_COLLAPSE },
	{ "GAUSSIAN_BLUR_KERNEL",	&config_buf.GAUSSIAN_BLUR_KERNEL },
	{ "MORPH_OPEN_KERNEL",		&config_buf.MORPH_OPEN_KERNEL },
	{ "MORPH_CLOSE_KERNEL",		&config_buf.MORPH_CLOSE_KERNEL },
	{ "THRESHOLD_THRESH",		&config_buf.THRESHOLD_THRESH },
	{ "THRESHOLD_MAXVAL",		&config_buf.THRESHOLD_MAXVAL },
	{ "WEB_SHOW_LINES",			&config_buf.WEB_SHOW_LINES },
	{ "WEB_INTERVAL",			&config_buf.WEB_INTERVAL },
	{ "BARCODE_LEFT",			&config_buf.BARCODE_LEFT },
	{ "BARCODE_WIDTH",			&config_buf.BARCODE_WIDTH },
	{ "DATAMATRIX_WIDTH",		&config_buf.DATAMATRIX_WIDTH },
	{ "LINE_WIDTH",		&config_buf.LINE_WIDTH },
	{ "CAM_TIMEOUT",	&config_buf.CAM_TIMEOUT },
	{ "CALIBRATE_CAM",	&config_buf.CALIBRATE_CAM }
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

void read_calibration() {

	string str(calibration_filename);
	write_log("calibration_filename = " + str);

	cv::FileStorage fs(str, cv::FileStorage::READ);

	fs["cameraMatrix"] >> cameraMatrix;
	fs["distCoeffs"] >> distCoeffs;

	fs.release();

}

void read_config()
{
	string str(config_filename);
	write_log("config_filename = " + str);

	Config cfg;

	try
	{
		cfg.readFile(config_filename.c_str());
	}
	catch(const FileIOException &fioex)
	{
		write_err(str + " I/O error while reading file.");
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
		config.recount_data_size();

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

	Config cfg;

	try
	{
		cfg.readFile(config_filename.c_str());
	}
	catch(const FileIOException &fioex)
	{
		write_err("I/O error while reading file.");
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
		cfg.writeFile(config_filename.c_str());
	}
	catch(const FileIOException &fioex)
	{
		string str(fioex.what());
		write_log("I/O error while writing file: " + str);
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

