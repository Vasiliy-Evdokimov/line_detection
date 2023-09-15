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

pthread_t udp_thread_id = 0;

int sockfd = -1;

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

	uint16_t counter = 0;

	udp_package udp_pack;

	while (true) {

		if (restart_threads || kill_threads) break;

		int numBytes = recvfrom( sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &clientAddr, &addrLen );
		if (numBytes < 0) {
			std::cerr << "data receiving error!\n";
			return;
		}

		//	std::cout << "Received: " << std::string(buffer, numBytes) << std::endl;

		counter++;

		udp_pack.counter = counter;
		for (int i = 0; i < 2; i++) {
			parse_results_mtx[i].lock();
			udp_pack.results[i] = parse_results[i];
			parse_results_mtx[i].unlock();
		}

		size_t sz = sizeof(udp_pack);
		if (UDP_LOG) {
			char* my_s_bytes = reinterpret_cast<char*>(&udp_pack);
			for (size_t i = 0; i < sz; i++)
				printf("%02x ", my_s_bytes[i]);
			printf("\n");
		}

		if ( sendto(sockfd, &udp_pack, sizeof(udp_pack), 0, (struct sockaddr *) &clientAddr, addrLen) == -1 )
		{
			fprintf(stderr, "sendto()");
			exit(1);
		}

	}

	std::cout << "UDP out of infinity loop.\n";

	close(sockfd);

}
