#include <thread>

using namespace std;

#include "config.hpp"
#include "camera.hpp"
#include "udp.hpp"
#include "http.hpp"

bool kill_threads = false;

void work_func()
{
	do {

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

		cout << "restart_threads = " << restart_threads << endl;

		restart_threads = false;

	} while (!kill_threads);
}

int main(int argc, char** argv)
{
	//	читаем параметры из конфигурационного файла
	read_config(argv[0]);
	//	создаем рабочий поток
	thread work_thread(work_func);
	//	инициализируем HTTP
    http_init();
    //
    work_thread.joinable();
    //
    return 0;
}
