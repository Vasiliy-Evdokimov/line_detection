/*
 * config_path.cpp
 *
 *  Created on: 11 дек. 2023 г.
 *      Author: vevdokimov
 */

#include <string>

#include <sys/types.h>

#ifdef _WIN32
	#include <direct.h>
#elif __linux__
	#include <unistd.h>
#endif

#include "config_path.hpp"

using namespace std;

const string debug_config_directory =
	"/home/vevdokimov/eclipse-workspace/line_detection/config/";

string get_work_directory()
{
	string result = "";

#ifdef _WIN32
	char* cwd;
	if ((cwd = _getcwd(NULL, 0)) != nullptr)
	{
		result.append(cwd);
		result.append("\\");
	}
	else
	{
		perror("_getcwd() error");
	}
#elif __linux__
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) != nullptr)
	{
		result.append(cwd);
		result.append("/");
	}
	else
	{
		perror("getcwd() error");
	}
#endif

	return result;
}

string get_config_directory()
{
	string result = get_work_directory();
	result.append("config/");
	return result;
}

string get_actual_config_directory()
{
	return
		#ifdef RELEASE
			get_config_directory();
		#else
			debug_config_directory;
		#endif
}
