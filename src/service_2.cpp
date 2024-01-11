#include "service_2.hpp"

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
	cout << getpid() << " " << service_name << " "
		 << time << " " << tag << " " << msg << endl;
}

//

static LDService *impl = nullptr;
static string TAG = "MAIN";
static char pidFileName[1024];
static string START = "-start";
static string STOP = "-stop";
static string RESTART = "-restart";

static void storePid(string pidFileName, int val)
{
	FILE *f = fopen(pidFileName.c_str(), "w+");
	if(f != nullptr)
	{
		fprintf(f, "%i", val);
		fflush(f);
		fclose(f);
	}
}

static int restorePid(string pidFileName)
{
	int ret = -1;
	FILE *f = fopen(pidFileName.c_str(), "r");
	if(f != nullptr)
	{
		fscanf(f, "%i", &ret);
		fclose(f);
	}
	return ret;
}

static void removePid(string pidFileName)
{
	unlink(pidFileName.c_str());
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
	removePid(pidFileName);
	exit(-1);
}

static void serviceFunc()
{
	std::cout << "serviceFunc()" << std::endl;

	static struct sigaction sigact;
	sigset_t         sigset;
	int              signo;

	storePid(pidFileName, getpid());

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
	removePid(pidFileName);
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

	if(impl)
		impl->logMsg(tag, timestr, buff);
	else
		printf("\"%-20s\"[%s] - %s", tag.c_str(), timestr, buff);
}

int service_main(int argc, char** args, LDService* iService)
{
	pid_t pid = 0;
	int status = 0;

	if(!iService)
	{
		printf("Interface is empty\n");
		return -1;
	}

	impl = iService;
	// если в аргументах не указано действие, то выводим подсказку
	if (argc < 2)
	{
		printf("%s [OPTION]\n", impl->serviceName().c_str());
		printf("\t-start\t\tstart service\n");
		printf("\t-restart\trestart service\n");
		printf("\t-stop\t\tstop service\n");
		impl = nullptr;
		return -1;
	}
	// формируем имя файла, содержащего PID процесса
	sprintf(pidFileName, "/tmp/%s.pid", impl->serviceName().c_str());
	// читаем из файла PID
	pid = restorePid(pidFileName);

	if(!strncmp(args[1], RESTART.c_str(), strlen(RESTART.c_str())))
	{
		if(pid == -1)
		{
			printf("Service is not started\n");
			return -1;
		}
		kill(pid, SIGUSR1);
		waitpid(pid, &status, 0);
		status = (((status) & 0xff00) >> 8);
		printf("Try restart service: %i ST:%i\n", pid, status);
	}
	else if(!strncmp(args[1], STOP.c_str(), strlen(STOP.c_str())))
	{
		if(pid == -1)
		{
			printf("Service is already stopped\n");
			return 0;
		}
		kill(pid, SIGINT);
		waitpid(pid, &status, 0);
		status = (((status) & 0xff00) >> 8);
		printf("Try stop service: %i ST:%i\n", pid, status);
	}
	else if(!strncmp(args[1], START.c_str(), strlen(START.c_str())))
	{
		if(pid != -1)
		{
			printf("Service is already started %i\n", pid);
			return 0;
		}
		// создаем копию процесса
		pid_t _pid = fork();
		if(_pid == -1)
		{
			printf("Start error\n");
			return -1;
		}
		// pid != 0 - значит это код родительского процесса;
		// сообщаем пользователю, что стартуем сервис
		else if(_pid != 0)
		{
			printf("Starting service %i\n", _pid);
		}
		// pid == 0 - значит это код дочернего процесса, который и будет "рабочим"
		else
		{
			umask(0);
			chdir("/");
//			close(STDIN_FILENO);
//			close(STDOUT_FILENO);
//			close(STDERR_FILENO);
			setsid();
			//	???
			pid_t fpid = fork();
			if(!fpid)
			{
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
