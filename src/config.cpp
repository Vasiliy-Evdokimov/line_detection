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

bool restart_threads;
bool kill_threads;

void recount_data_size(ConfigData& cfg) {
	cfg.DATA_SIZE =
		(cfg.NUM_ROI_H * cfg.NUM_ROI_V + (cfg.NUM_ROI - cfg.NUM_ROI_H));
}

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

		string buf1 = cfg.lookup("CAM_ADDR_1");
		config.CAM_ADDR_1 = buf1;
		string buf2 = cfg.lookup("CAM_ADDR_2");
		config.CAM_ADDR_2 = buf2;

		string udp_addr_str = cfg.lookup("UDP_ADDR");
		strcpy(config.UDP_ADDR, udp_addr_str.c_str());

		config.UDP_PORT = cfg.lookup("UDP_PORT");

		config.NUM_ROI = cfg.lookup("NUM_ROI");
		config.NUM_ROI_H = cfg.lookup("NUM_ROI_H");
		config.NUM_ROI_V = cfg.lookup("NUM_ROI_V");
		recount_data_size(config);

		config.SHOW_GRAY = cfg.lookup("SHOW_GRAY");
		config.DETAILED = cfg.lookup("DETAILED");
		config.DRAW_GRID = cfg.lookup("DRAW_GRID");
		config.DRAW = cfg.lookup("DRAW");

		//	минимальная длина контура
		config.MIN_CONT_LEN = cfg.lookup("MIN_CONT_LEN");
		//	при этом или меньшем расстоянии между горизонтальными линиями, они усредняются в одну
		config.HOR_COLLAPSE = cfg.lookup("HOR_COLLAPSE");

		config.GAUSSIAN_BLUR_KERNEL = cfg.lookup("GAUSSIAN_BLUR_KERNEL");
		config.MORPH_OPEN_KERNEL = cfg.lookup("MORPH_OPEN_KERNEL");
		config.MORPH_CLOSE_KERNEL = cfg.lookup("MORPH_CLOSE_KERNEL");

		config.THRESHOLD_THRESH = cfg.lookup("THRESHOLD_THRESH");
		config.THRESHOLD_MAXVAL = cfg.lookup("THRESHOLD_MAXVAL");

		restart_threads = false;
		kill_threads = false;
	}
	catch(const SettingNotFoundException &nfex)
	{
		cerr << "Critical setting was not found in configuration file.\n";
		exit(1);
	}

}
