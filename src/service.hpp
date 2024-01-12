#pragma once

#include <string>

using namespace std;

struct ServiceHandlers
{
	int (*onLoadConfigPtr)() = nullptr;
	int (*onStartPtr)() = nullptr;
	int (*onRestartPtr)() = nullptr;
	void (*onDestroyPtr)() = nullptr;
};

class LDService
{
private:
	string service_name = "";
	ServiceHandlers handlers;
public:
	LDService(string aName, ServiceHandlers aHandlers)
	{
		service_name = aName;
		handlers = aHandlers;
	}
	//
	string serviceName() { return service_name; }
	void logMsg(string tag, string time, string msg);
	int onLoadConfig() { return handlers.onLoadConfigPtr(); }
	int onStart() { return handlers.onStartPtr(); }
	int onRestart() { return handlers.onRestartPtr(); }
	void onDestroy() { handlers.onDestroyPtr(); }
};

int service_main(int argc, char** args, LDService* iService);
void service_log_msg(string tag, string fmt, ...);
