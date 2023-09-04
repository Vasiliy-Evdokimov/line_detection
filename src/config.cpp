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

void read_config(char* exe) {

	Config cfg;

	char cfg_filename[255];

	sprintf(cfg_filename, "%s.cfg", exe);

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

	try {

		config.roi_amount = cfg.lookup("roi_amount");
		config.roi_h_amount = cfg.lookup("roi_h_amount");
		config.roi_v_amount = cfg.lookup("roi_v_amount");

		string buf1 = cfg.lookup("camera_address_1");
		config.cam_addr_1 = buf1;
		string buf2 = cfg.lookup("camera_address_2");
		config.cam_addr_2 = buf2;

		string udp_addr_str = cfg.lookup("udp_address");
		strcpy(config.udp_addr, udp_addr_str.c_str());

		config.udp_port = cfg.lookup("udp_port");

	}
	catch(const SettingNotFoundException &nfex)
	{
		cerr << "Critical setting was not found in configuration file.\n";
		exit(1);
	}

}
