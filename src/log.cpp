/*
 * log.cpp
 *
 *  Created on: 29 сент. 2023 г.
 *      Author: vevdokimov
 */

#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "log.hpp"

using namespace std;

string GetCurrentTime() {
	time_t currentTime = chrono::system_clock::to_time_t(chrono::system_clock::now());
	//
	stringstream ss;
	ss << put_time(localtime(&currentTime), "%Y-%m-%d %H:%M:%S");
	string currentTimeString = ss.str();
	//
	return currentTimeString;
}

void write_log(string aMessage)
{
	cout << GetCurrentTime() << ": " << aMessage << endl;
}

void write_err(string aMessage)
{
	cerr << GetCurrentTime() << ": " << aMessage << endl;
}
