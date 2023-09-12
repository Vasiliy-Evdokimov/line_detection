#include <thread>
#include <csignal>

using namespace std;

#include "config.hpp"
#include "shared_memory.hpp"
#include "camera.hpp"
#include "udp.hpp"
#include "http.hpp"

void work_func()
{
	kill_threads = false;

	while (!kill_threads) {

		//	создаем потоки для камер
		thread cam1_thread(camera_func, "Cam1", config.CAM_ADDR_1, 0);
		std::this_thread::sleep_for(2s);
		thread cam2_thread(camera_func, "Cam2", config.CAM_ADDR_2, 1);
		std::this_thread::sleep_for(2s);
		//	создаем поток для передачи по UDP
		thread udp_thread(udp_func);
		//
		if (cam1_thread.joinable()) cam1_thread.join();
		if (cam2_thread.joinable()) cam2_thread.join();
		if (udp_thread.joinable()) udp_thread.join();

		restart_threads = false;

	};
}

void signalHandler( int signum ) {

	cout << "Interrupt signal (" << signum << ") received.\n";

	http_quit();
	//
	kill_threads = true;

	exit(signum);

}

int main(int argc, char** argv)
{
	//signal(SIGINT, signalHandler);
	//
	//	читаем параметры из конфигурационного файла
	read_config(argv[0]);
	//
	init_shared_memory();
	//
	//	создаем рабочий поток
	thread work_thread(work_func);
	//	создаем поток визуализации
	thread visualizer_thread(visualizer_func);
	//	инициализируем HTTP
	//	http_init();
    //
    if (work_thread.joinable()) work_thread.join();
    if (visualizer_thread.joinable()) visualizer_thread.join();
    //
    return 0;
}
