#include <thread>
#include <csignal>

using namespace std;

#include "defines.hpp"
#include "config.hpp"
#include "shared_memory.hpp"
#include "camera.hpp"
#include "udp.hpp"

void work_func()
{

	pthread_setname_np(pthread_self(), "work thread");

	kill_threads = false;

	cout << "work_func() started!\n";
	cout << "work_func() entered infinity loop.\n";

	thread cam_threads[CAM_COUNT];

	while (!kill_threads) {

		init_config_sm(config);
		//
		string cam_addresses[CAM_COUNT] = { config.CAM_ADDR_1, config.CAM_ADDR_2 };
		//
		cout << "config_sm_id = " << config_sm_id << endl;
		cout << "config_sm_ptr = " << config_sm_ptr << endl;
		cout << "config_sm_ptr->PID = " << config_sm_ptr->PID << endl;
		//
		//	создаем потоки для камер
		for (int i = 0; i < CAM_COUNT; i++) {
			cam_threads[i] = thread(camera_func, "Cam" + to_string(i + 1), cam_addresses[i], i);
			std::this_thread::sleep_for(1s);
		}
		//	создаем поток для передачи по UDP
		thread udp_thread(udp_func);
		udp_thread_id = udp_thread.native_handle();
		//
		for (int i = 0; i < CAM_COUNT; i++)
			if (cam_threads[i].joinable()) cam_threads[i].join();
		if (udp_thread.joinable()) udp_thread.join();
		//
		restart_threads = false;

	};

	cout << "work_func() is out of infinity loop.\n";

}

void signalHandler( int signum ) {

	if (signum == SIGINT) {
		cout << "SIGINT received.\n";
		//
		kill_udp_thread();
		kill_threads = true;
	} else
	//
	if (signum == SIGUSR1) {
		cout << "SIGUSR1 received.\n";
		//
		kill_udp_thread();
		restart_threads = true;
	}

}

int main(int argc, char** argv)
{

	cout << "Application started!\n";
	//
	signal(SIGINT, signalHandler);
	signal(SIGUSR1, signalHandler);
	//
	//	читаем параметры из конфигурационного файла
	read_config();
	//
	//	инициализируем shared memory
	init_shared_memory();
	//
	//	создаем рабочий поток
	thread work_thread(work_func);
	std::this_thread::sleep_for(1s);
	//
#ifndef NO_GUI
	//	создаем поток визуализации
	thread visualizer_thread(visualizer_func);
#endif
    //
    if (work_thread.joinable()) work_thread.join();
    //
#ifndef NO_GUI
    if (visualizer_thread.joinable()) visualizer_thread.join();
#endif
    //
    //	бесконечный цикл для ожидания сигналов
    while (!kill_threads) {
    	//
    }
    //
    cout << "Application terminated!\n";
    //
    return 0;

}
