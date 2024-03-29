/*
 * log.hpp
 *
 *  Created on: 29 сент. 2023 г.
 *      Author: vevdokimov
 */

#ifndef LOG_HPP_
#define LOG_HPP_

#include <string>

using namespace std;

extern string log_filename;

string GetCurrentTime(char* fmt);

void write_log(string aMessage);
void write_err(string aMessage);

#endif /* LOG_HPP_ */
