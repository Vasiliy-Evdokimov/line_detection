/*
 * udp.cpp
 *
 *  Created on: Aug 16, 2023
 *      Author: vevdokimov
 */

#include <iostream>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>

#include <thread>

#include "defines.hpp"
#include "config.hpp"
#include "camera.hpp"

#include <pthread.h>

#include "udp.hpp"

#include "string.h"

#define UDP_BUFLEN 255

using namespace std;

udp_package udp_packs[2];

pthread_t udp_thread_id = 0;

int sockfd = -1;

void make_udp_pack(int aCounter, udp_package& udp_pack, ParseImageResult& parse_result) {

		memset(&udp_pack, 0, sizeof(udp_pack));

		udp_pack.counter = aCounter;
		udp_pack.img_width = parse_result.width;
		udp_pack.img_height = parse_result.height;
		udp_pack.error_flag = (parse_result.fl_error) ? 1 : 0;
		//
		udp_pack.points_count = parse_result.hor_ys.size() & 0xF;
		udp_pack.points_count |= (parse_result.res_points.size() & 0xF) << 4;
		//
		for (size_t i = 0; i < parse_result.res_points.size(); i++)
		{
			if (UDP_LOG) printf("(%d; %d) ", parse_result.res_points[i].x, parse_result.res_points[i].y);
			udp_pack.points[i] = { (uint16_t)parse_result.res_points[i].x, (uint16_t)parse_result.res_points[i].y };
		}
		if (UDP_LOG) printf("\n");
		//
		for (size_t i = 0; i < parse_result.hor_ys.size(); i++)
		{
			if (UDP_LOG) printf("(%d) ", parse_result.hor_ys[i]);
			udp_pack.points_hor[i] = { (uint16_t)parse_result.hor_ys[i] };
		}
		if (UDP_LOG) printf("\n");
		//
		size_t sz = sizeof(udp_pack);
		if (UDP_LOG) {
			char* my_s_bytes = reinterpret_cast<char*>(&udp_pack);
			for (size_t i = 0; i < sz; i++)
				printf("%02x ", my_s_bytes[i]);
			printf("\n");
		}

}

void kill_udp_thread()
{
	if (udp_thread_id)
		pthread_cancel(udp_thread_id);
}

void udp_func()
{

	std::cout << "UPD thread started!\n";

	struct sockaddr_in serverAddr{}, clientAddr{};
	socklen_t addrLen = sizeof(clientAddr);

	if (sockfd >= 0)
	try {
		close(sockfd);
	} catch (...) {
		cout << "socket close error!\n";
	}
	//
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		std::cerr << "socket creating error!\n";
		return;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(config.UDP_PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
		std::cerr << "socket binding error!\n";
		return;
	}

	char buffer[UDP_BUFLEN];

	std::cout << "UDP thread entered infinity loop.\n";

	ParseImageResult pr[2];

	uint16_t counter = 0;

	while (true) {

		if (restart_threads || kill_threads) break;

		int numBytes = recvfrom( sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &clientAddr, &addrLen );
		if (numBytes < 0) {
			std::cerr << "data receiving error!\n";
			return;
		}

		//	std::cout << "Received: " << std::string(buffer, numBytes) << std::endl;

		for (int i = 0; i < 2; i++) {
			parse_results_mtx[i].lock();
			pr[i] = parse_results[i];
			parse_results_mtx[i].unlock();
		}

		counter++;

		for (int i = 0; i < 2; i++)
			make_udp_pack(counter, udp_packs[i], parse_results[i]);

		size_t sz = sizeof(udp_packs);
		if (UDP_LOG) {
			char* my_s_bytes = reinterpret_cast<char*>(&udp_packs);
			for (size_t i = 0; i < sz; i++)
				printf("%02x ", my_s_bytes[i]);
			printf("\n");
		}

		if ( sendto(sockfd, &udp_packs, sizeof(udp_packs), 0, (struct sockaddr *) &clientAddr, addrLen) == -1 )
		{
			fprintf(stderr, "sendto()");
			exit(1);
		}

	}

	std::cout << "UDP out of infinity loop.\n";

	close(sockfd);

}
