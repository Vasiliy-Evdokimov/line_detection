#include <thread>

using namespace std;

#include "config.hpp"
#include "camera.hpp"
#include "udp.hpp"

int main(int argc, char** argv)
{
	read_config(argv[0]);
	//
    thread cam1_thread(camera_func, "Cam1_", config.cam_addr_1, 0);
    std::this_thread::sleep_for(2s);
    thread cam2_thread(camera_func, "Cam2_", config.cam_addr_2, 1);
    std::this_thread::sleep_for(2s);
    thread udp_thread(udp_func);
    //
    if (cam1_thread.joinable()) cam1_thread.join();
    if (cam2_thread.joinable()) cam2_thread.join();
    if (udp_thread.joinable()) udp_thread.join();
    //
    return 0;
}
