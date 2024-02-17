/*
 * udp.cpp
 *
 *  Created on: Aug 16, 2023
 *      Author: vevdokimov
 */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>

#include <thread>

#include "defines.hpp"
#include "config.hpp"
#include "log.hpp"
#include "camera.hpp"
#include "shared_memory.hpp"

#include <pthread.h>

#include "udp.hpp"

#include "string.h"
#include "crc.hpp"

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

	pthread_setname_np(pthread_self(), "udp thread");

	write_log("UPD thread started!");

	struct sockaddr_in serverAddr{}, clientAddr{};
	socklen_t addrLen = sizeof(clientAddr);

	if (sockfd >= 0)
	try {
		close(sockfd);
	} catch (...) {
		write_err("socket close error!");
	}
	//
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		write_err("socket creating error!");
		return;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(config.UDP_PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
		write_err("socket binding error!");
		return;
	}

	char buffer[UDP_BUFLEN];

	write_log("UDP thread entered infinity loop.");

	uint16_t counter = 0;

	udp_package udp_pack;

	while (true) {

		if (restart_threads || kill_threads) break;

		int numBytes = recvfrom( sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &clientAddr, &addrLen );
		if (numBytes < 0) {
			write_err("data receiving error!");
			return;
		}

		string receivedReq = string(buffer, numBytes);
		//write_log("Received: " + receivedReq);

		if (receivedReq != config.UDP_REQUEST)
			continue;

		counter++;

		udp_pack.counter = counter;

		ResultFixed results_buf[CAM_COUNT];

		for (int i = 0; i < CAM_COUNT; i++)
			read_results_sm(results_buf[i], i);

		for (int i = 0; i < CAM_COUNT; i++)
			std::memcpy(&udp_pack.results[UDP_RESULT_SIZE * i], &results_buf[i], UDP_RESULT_SIZE);

		uint8_t buf[sizeof(udp_pack) - 2];
		std::memcpy(&buf, &udp_pack, sizeof(buf));
		udp_pack.crc = crc16(buf, sizeof(buf));

		size_t sz = sizeof(udp_pack);

#ifdef UDP_LOG
		char* my_s_bytes = reinterpret_cast<char*>(&udp_pack);
		for (size_t i = 0; i < sz; i++)
			printf("%02x ", my_s_bytes[i]);
		printf("\n");
#endif

		if ( sendto(sockfd, &udp_pack, sizeof(udp_pack), 0, (struct sockaddr *) &clientAddr, addrLen) == -1 )
		{
			fprintf(stderr, "sendto()");
			exit(1);
		}

	}

	write_log("UDP out of infinity loop.");

	close(sockfd);

}
