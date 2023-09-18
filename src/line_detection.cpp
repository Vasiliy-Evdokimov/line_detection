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

	if (THREAD_NAMING)
		pthread_setname_np(pthread_self(), "work thread");

	kill_threads = false;

	cout << "work_func() started!\n";
	cout << "work_func() entered infinity loop.\n";

	while (!kill_threads) {

		init_config_sm(config);
		//
		cout << "config_sm_id = " << config_sm_id << endl;
		cout << "config_sm_ptr = " << config_sm_ptr << endl;
		cout << "config_sm_ptr->PID = " << config_sm_ptr->PID << endl;
		//
		//	создаем потоки для камер
		thread cam1_thread(camera_func, "Cam1", config.CAM_ADDR_1, 0);
		std::this_thread::sleep_for(2s);
		thread cam2_thread(camera_func, "Cam2", config.CAM_ADDR_2, 1);
		std::this_thread::sleep_for(2s);
		//	создаем поток для передачи по UDP
		thread udp_thread(udp_func);
		udp_thread_id = udp_thread.native_handle();
		//
		if (cam1_thread.joinable()) cam1_thread.join();
		if (cam2_thread.joinable()) cam2_thread.join();
		if (udp_thread.joinable()) udp_thread.join();

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
	std::this_thread::sleep_for(2s);
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
