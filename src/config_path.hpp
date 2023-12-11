/*
 * config_path.hpp
 *
 *  Created on: 11 дек. 2023 г.
 *      Author: vevdokimov
 */

#ifndef CONFIG_PATH_HPP_
#define CONFIG_PATH_HPP_

#include <string>

using namespace std;

extern const string debug_config_directory;

string get_work_directory();
string get_config_directory();
string get_actual_config_directory();

#endif /* CONFIG_PATH_HPP_ */
