#include "defines.hpp"
#include "log.hpp"
#include "service.hpp"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>
#include <time.h>

#include <iostream>

void LDService::logMsg(string tag, string time, string msg)
{
	write_log(
		//to_string(getpid()) + " " + service_name + " " + time + " " +
		tag + " " + msg
	);
}

//

static LDService *impl = nullptr;
static string TAG = "MAIN";
static string START = "-start";
static string STOP = "-stop";
static string RESTART = "-restart";

static int getPid()
{
  int ret = -1;
  if (!impl)
    return ret;

  char buf[128];
  FILE *f = nullptr;

  snprintf(buf, 128, "pidof %s", (impl->serviceName()).c_str());
  service_log_msg(TAG, "getPid() buf = %s", buf);
  f = popen(buf, "r");
  if (f != NULL)
  {
    fgets(buf, 128, f);
    pclose(f);

    pid_t thisPid = getpid(), pid1 = -1, pid2 = -1;
    sscanf(buf, "%i %i", &pid1, &pid2);
    if(pid1 == thisPid)
      ret = pid2;
    else if(pid2 == thisPid)
      ret = pid1;
  }
  return ret;
}

static void sigProc(int sig, siginfo_t *si, void *ptr)
{
	void*  ErrorAddr;
	void*  Trace[16];
	int    x;
	int    TraceSize;
	char** Messages;

	service_log_msg(TAG, "Signal: %s, Addr: 0x%p", strsignal(sig), si->si_addr);

#ifdef __ARM_64BIT_STATE
	ErrorAddr = (void*)((ucontext_t*)ptr)->uc_mcontext.fault_address;
#else
	#if __WORDSIZE == 64
		ErrorAddr = (void*)((ucontext_t*)ptr)->uc_mcontext.gregs[REG_RIP];
	#else
		ErrorAddr = (void*)((ucontext_t*)ptr)->uc_mcontext.fault_address;
	#endif
#endif

	service_log_msg(TAG, "Fault Address: 0x%p", ErrorAddr);

	TraceSize = backtrace(Trace, 16);
	Trace[1] = ErrorAddr;
	Messages = backtrace_symbols(Trace, TraceSize);
	if (Messages)
	{
		service_log_msg(TAG, "== Backtrace ==");
		for (x = 1; x < TraceSize; x++)
			service_log_msg(TAG, "%s", Messages[x]);

		service_log_msg(TAG, "== End Backtrace ==");
		free(Messages);
	}
	service_log_msg(TAG, "Stopped with OS signal");

	impl->onDestroy();
	exit(-1);
}

static void serviceFunc()
{
	static struct sigaction sigact;
	sigset_t         sigset;
	int              signo;

	sigact.sa_flags = SA_SIGINFO;
	sigact.sa_sigaction = &sigProc;

	sigemptyset(&sigact.sa_mask);
	sigaction(SIGFPE, &sigact, nullptr);
	sigaction(SIGILL, &sigact, nullptr);
	sigaction(SIGSEGV, &sigact, nullptr);
	sigaction(SIGBUS, &sigact, nullptr);

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGQUIT);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGTERM);
	sigaddset(&sigset, SIGKILL);
	sigaddset(&sigset, SIGABRT);
	sigaddset(&sigset, SIGUSR1);
	sigprocmask(SIG_BLOCK, &sigset, nullptr);

	if(!impl->onLoadConfig())
	{
		service_log_msg(TAG, "Started");

		if (!impl->onStart())
		{
			service_log_msg(TAG, "Service entered infinity loop.");
			for (;;)
			{
				sigwait(&sigset, &signo);

				if (signo == SIGUSR1)
				{
					if (!impl->onRestart())
					{
						service_log_msg(TAG, "Reload OK");
					}
					else
					{
						service_log_msg(TAG, "Reload FAILED");
					}
				}
				else
				{
					service_log_msg(TAG, "Signal %i", signo);
					break;
				}
			}
			service_log_msg(TAG, "Service is out of infinity loop.");
			impl->onDestroy();
		}
		else
		{
			service_log_msg(TAG, "Init service failed");
		}
	}
	else
	{
		service_log_msg(TAG, "Load config failed");
	}
	service_log_msg(TAG, "Stopped");
	exit(0);
}

void service_log_msg(string tag, string fmt, ...)
{
	char timestr[30]={0}, buff[1024]={0};
	time_t t;

	va_list args;
	va_start(args, fmt);
	vsprintf(buff, fmt.c_str(), args);
	va_end(args);

	t = time(NULL);
	strftime(timestr, 30, "%Y-%m-%d %H:%M:%S", localtime(&t));

#ifndef RELEASE
	printf("%s [%s] - %s\n", tag.c_str(), timestr, buff);
#endif

	if(impl)
		impl->logMsg(tag, timestr, buff);
	else
		printf("%s [%s] - %s\n", tag.c_str(), timestr, buff);
}

int service_main(int argc, char** args, LDService* iService)
{
	pid_t pid = 0;
	int status = 0;

	if(!iService)
	{
		printf("Service handler is empty\n");
		return -1;
	}

	impl = iService;

	if (argc < 2)
	{
		printf("%s [OPTION]\n", impl->serviceName().c_str());
		printf("\t-start\t\tstart service\n");
		printf("\t-restart\trestart service\n");
		printf("\t-stop\t\tstop service\n");
		impl = nullptr;
		return -1;
	}

	pid = getPid();

	service_log_msg(TAG, "service_main(%s) getPid() = %d", args[1], pid);

	if(!strncmp(args[1], RESTART.c_str(), strlen(RESTART.c_str())))
	{
		if(pid == -1)
		{
			service_log_msg(TAG, "Service is not started");
			return -1;
		}
		kill(pid, SIGUSR1);
		waitpid(pid, &status, 0);
		status = (((status) & 0xff00) >> 8);
		//
		service_log_msg(TAG, "Try restart service: %i ST:%i", pid, status);
	}
	else if(!strncmp(args[1], STOP.c_str(), strlen(STOP.c_str())))
	{
		if(pid == -1)
		{
			service_log_msg(TAG, "Service is already stopped");
			return 0;
		}
		kill(pid, SIGINT);
		waitpid(pid, &status, 0);
		status = (((status) & 0xff00) >> 8);
		service_log_msg(TAG, "Try stop service: %i ST:%i", pid, status);
	}
	else if(!strncmp(args[1], START.c_str(), strlen(START.c_str())))
	{
		if(pid != -1)
		{
			service_log_msg(TAG, "Service is already started %i", pid);
			return 0;
		}

		pid_t _pid = fork();
		if(_pid == -1)
		{
			service_log_msg(TAG, "Start error\n");
			return -1;
		}
		else if(_pid != 0)
		{
			service_log_msg(TAG, "Starting service %i", _pid);
		}
		else
		{
			char dir[255];
			getcwd(dir, 255);
			//
			service_log_msg(TAG, "Current directory is %s", dir);
			//
			umask(0);
			chdir("/");
			//
#ifdef SERVICE
	#ifdef RELEASE
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);

	#endif
#endif
			//
			setsid();
//			pid_t fpid = fork();
//			if(!fpid)
			{
				chdir(dir);
				serviceFunc();
			}
		}
	}
	else
	{
		return -1;
	}

	return 0;
}
