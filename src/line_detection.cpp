#include <chrono>
#include <thread>
#include <pthread.h>
#include <csignal>

#include <unistd.h>
#include <stdio.h>

using namespace std;
using namespace std::chrono_literals;

#include "defines.hpp"
#include "config.hpp"
#include "config_path.hpp"
#include "log.hpp"
#include "shared_memory.hpp"
#include "camera.hpp"
#include "calibration.hpp"
#include "udp.hpp"
#include "templates.hpp"

#include "service.hpp"

void work_func()
{
	pthread_setname_np(pthread_self(), "work thread");

	kill_threads = false;

	write_log("work_func() started!");
	write_log("work_func() entered infinity loop.");

	thread cam_threads[CAM_COUNT];

	while (!kill_threads) {

		init_config_sm(config);
		//
		string cam_addresses[CAM_COUNT] = { config.CAM_ADDR_1, config.CAM_ADDR_2 };
		//
		//	создаем потоки для камер
		for (int i = 0; i < CAM_COUNT; i++) {
			if (!(config.USE_CAM & (1 << i))) continue;
			//
			cam_threads[i] = thread(camera_func, "Cam" + to_string(i + 1), cam_addresses[i], i);
			std::this_thread::sleep_for(1s);
		}
		//	создаем поток для передачи по UDP
		thread udp_thread(udp_func);
		udp_thread_id = udp_thread.native_handle();
		//
		for (int i = 0; i < CAM_COUNT; i++) {
			if (!(config.USE_CAM & (1 << i))) continue;
			//
			if (cam_threads[i].joinable()) cam_threads[i].join();
		}
		if (udp_thread.joinable()) udp_thread.join();
		//
		restart_threads = false;

	};

	write_log("work_func() is out of infinity loop.");
}

void signalHandler( int signum )
{
	if (signum == SIGINT) {
		write_log("SIGINT received.");
		//
		kill_udp_thread();
		kill_threads = true;
	} else
	//
	if (signum == SIGUSR1) {
		write_log("SIGUSR1 received.");
		//
		kill_udp_thread();
		restart_threads = true;
	}
}

int main_function()
{
	pthread_setname_np(pthread_self(), "main thread");

	write_log("Application started!");
	//
	write_log("Current work directory is " + get_work_directory());
	//
	signal(SIGINT, signalHandler);
	signal(SIGUSR1, signalHandler);
	//
	//	читаем параметры из конфигурационного файла
	read_config();
	//
	//	читаем параметры калибровки камеры
#ifdef USE_UNDISTORT
	read_calibration();
#endif
	//
	//	читаем опорные точки калибровки
	load_intersection_points();
	//
	//	читаем конфигурацию шаблонов
#ifdef USE_TEMPLATES
	templates_load_config();
#endif
	//
	//	инициализируем shared memory
	init_shared_memory();
	//
	//	создаем рабочий поток
	thread work_thread(work_func);
	std::this_thread::sleep_for(1s);
	//
	//	создаем поток визуализации
#ifndef NO_GUI
	thread visualizer_thread(visualizer_func);
#endif
	//
	if (work_thread.joinable()) work_thread.join();
	//
#ifndef NO_GUI
	if (visualizer_thread.joinable()) visualizer_thread.join();
#endif
	//
//	//	бесконечный цикл для ожидания сигналов
//	while (!kill_threads) {
//		//
//		this_thread::sleep_for(100ms);
//		//
//	}
//	//
//	write_log("Application terminated!");
	//
	return 0;
}

int onLoadConfig() { return 0; }

int onStart() {	return main_function(); }

int onRestart()
{
	kill_udp_thread();
	restart_threads = true;
	return 0;
}

void onDestroy()
{
	kill_udp_thread();
	kill_threads = true;
}

int main(int argc, char** argv)
{
	//main_function();
	//
	ServiceHandlers srvh {
		onLoadConfig,
		onStart,
		onRestart,
		onDestroy
	};
	//
	LDService srv("service_name_1", srvh);
	service_main(argc, argv, &srv);
    //
    return 0;
}
