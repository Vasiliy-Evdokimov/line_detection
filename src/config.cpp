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

void recount_data_size() {
	config.DATA_SIZE =
		(config.NUM_ROI_H * config.NUM_ROI_V + (config.NUM_ROI - config.NUM_ROI_H));
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

		string buf1 = cfg.lookup("camera_address_1");
		config.CAM_ADDR_1 = buf1;
		string buf2 = cfg.lookup("camera_address_2");
		config.CAM_ADDR_2 = buf2;

		string udp_addr_str = cfg.lookup("udp_address");
		strcpy(config.UDP_ADDR, udp_addr_str.c_str());

		config.UDP_PORT = cfg.lookup("udp_port");

		config.NUM_ROI = cfg.lookup("roi_amount");
		config.NUM_ROI_H = cfg.lookup("roi_h_amount");
		config.NUM_ROI_V = cfg.lookup("roi_v_amount");
		recount_data_size();

		config.SHOW_CAM = cfg.lookup("show_camera");
		config.SHOW_GRAY = cfg.lookup("show_gray");
		config.DETAILED = cfg.lookup("draw_detailed");
		config.DRAW_GRID = cfg.lookup("draw_grid");
		config.DRAW = cfg.lookup("draw");

		//	минимальная длина контура
		config.MIN_CONT_LEN = cfg.lookup("minimal_contour_length");
		//	при этом или меньшем расстоянии между горизонтальными линиями, они усредняются в одну
		config.HOR_COLLAPSE = cfg.lookup("horizontal_collapse");

		config.GAUSSIAN_BLUR_KERNEL = cfg.lookup("gaussian_blur_kernel");
		config.MORPH_OPEN_KERNEL = cfg.lookup("morph_open_kernel");
		config.MORPH_CLOSE_KERNEL = cfg.lookup("morph_close_kernel");

		config.THRESHOLD_THRESH = cfg.lookup("threshold_thresh");
		config.THRESHOLD_MAXVAL = cfg.lookup("threshold_maxval");

	}
	catch(const SettingNotFoundException &nfex)
	{
		cerr << "Critical setting was not found in configuration file.\n";
		exit(1);
	}

}
