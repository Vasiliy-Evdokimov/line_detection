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

pthread_t p_work_thread;
pthread_t p_visualizer_thread;

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

void* p_work_func(void *args) { work_func(); return 0; }

int main_function()
{
	write_log("Current work directory is " + get_work_directory());
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
	pthread_create(&p_work_thread, NULL, p_work_func, NULL);
	//
	//	создаем поток визуализации
#ifndef NO_GUI
	pthread_create(&p_visualizer_thread, NULL, p_visualizer_func, NULL);
#endif
	//
#ifndef SERVICE
	//
	while (!kill_threads)
	{
		this_thread::sleep_for(100ms);
	}
	//
#endif
	//
	return 0;
}

int onLoadConfig()
{
	write_log("onLoadConfig()");
	return 0;
}

int onStart()
{
	write_log("onStart()");
	return main_function();
}

int onRestart()
{
	write_log("onRestart()");
	kill_udp_thread();
	restart_threads = true;
	return 0;
}

void onDestroy()
{
	write_log("onDestroy()");
	kill_udp_thread();
	kill_threads = true;
	pthread_join(p_work_thread, NULL);
#ifndef NO_GUI
	pthread_join(p_visualizer_thread, NULL);
#endif
}

int main(int argc, char** argv)
{
	pthread_setname_np(pthread_self(), "main thread");
	//
	write_log("Application started!");
	//
#ifndef SERVICE
	main_function();
#else
	ServiceHandlers srvh {
		onLoadConfig,
		onStart,
		onRestart,
		onDestroy
	};
	//
	LDService srv("line_detection", srvh);
	service_main(argc, argv, &srv);
#endif
    //
	write_log("Application terminated!");
	//
    return 0;
}
