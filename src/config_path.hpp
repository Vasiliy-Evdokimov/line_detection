/*
 * config_path.hpp
 *
 *  Created on: 11 дек. 2023 г.
 *      Author: vevdokimov
 */

#ifndef CONFIG_PATH_HPP_
#define CONFIG_PATH_HPP_

#include <string>

#include "defines.hpp"

using namespace std;

extern const string debug_config_directory;

string get_work_directory();
string get_config_directory();
string get_logs_directory();

#endif /* CONFIG_PATH_HPP_ */
