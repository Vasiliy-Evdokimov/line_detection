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

const string debug_work_directory = "/home/vevdokimov/eclipse-workspace/line_detection/";
const string release_work_directory = "/usr/bin/line_detection/";

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
	#ifdef SERVICE
		#ifdef RELEASE
			result = release_work_directory;
		#else
			result = debug_work_directory;
		#endif
	#else
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
#endif

	return result;
}

string get_config_directory()
{
	return
		#ifdef STANDALONE
			get_work_directory();
		#else
			get_work_directory() + "config/";
		#endif
}

string get_logs_directory()
{
	string result = get_work_directory();
	result.append("logs/");
	return result;
}
