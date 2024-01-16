/*
 * log.cpp
 *
 *  Created on: 29 сент. 2023 г.
 *      Author: vevdokimov
 */

#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <sys/types.h>
#include <unistd.h>

#include <mutex>

#include <string.h>

#include "defines.hpp"
#include "log.hpp"

mutex log_mtx;

using namespace std;

string log_filename = "";
streambuf *cout_old = nullptr;

string GetCurrentTime()
{
	time_t currentTime = chrono::system_clock::to_time_t(chrono::system_clock::now());
	//
	stringstream ss;
	struct tm timeinfo = {};

#ifdef _WIN32
	localtime_s(&timeinfo, &currentTime);
#elif __linux__
	timeinfo = *localtime(&currentTime);
#endif

	ss << put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
	string currentTimeString = ss.str();
	//
	return currentTimeString;
}

void write_log(string aMessage)
{
	log_mtx.lock();
	//
#ifdef SERVICE
	ofstream fout(log_filename, ios_base::app);
	cout_old = cout.rdbuf();
	cout.rdbuf(fout.rdbuf());
#endif
	cout << "PID " << to_string(getpid())
		 << ": " << GetCurrentTime()
		 <<	": " << aMessage << endl;
#ifdef SERVICE
	cout.rdbuf(cout_old);
	fout.close();
#endif
	//
	log_mtx.unlock();
}

void write_err(string aMessage)
{
	cerr << GetCurrentTime() << ": " << aMessage << endl;
}
