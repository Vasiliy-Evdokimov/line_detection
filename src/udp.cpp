/*
 * udp.cpp
 *
 *  Created on: Aug 16, 2023
 *      Author: vevdokimov
 */

#include <iostream>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <thread>

#include "defines.hpp"
#include "udp.hpp"
#include "config.hpp"

#include "string.h"

using namespace std;

mutex udp_packs_mtx;
udp_package udp_packs[2];

void udp_func()
{

	std::cout << "UPD thread started!\n";

	struct sockaddr_in si_other;
	int sock, slen = sizeof(si_other);

	if ( (sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 )
	{
		fprintf(stderr, "socket");
		exit(1);
	}

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(config.UDP_PORT);

	char udp_addr[15];
	strcpy(udp_addr, config.UDP_ADDR.c_str());
	if (inet_aton(udp_addr, &si_other.sin_addr) == 0)
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	std::cout << "UDP thread entered infinity loop.\n";

	udp_package buf_udp_packs[2];

	while (!restart_threads) {

		udp_packs_mtx.lock();
		memcpy(buf_udp_packs, udp_packs, sizeof(buf_udp_packs));
		udp_packs_mtx.unlock();

		size_t sz = sizeof(buf_udp_packs);
		if (UDP_LOG) {
			char* my_s_bytes = reinterpret_cast<char*>(&buf_udp_packs);
			for (size_t i = 0; i < sz; i++)
				printf("%02x ", my_s_bytes[i]);
			printf("\n");
		}

		if ( sendto(sock, &buf_udp_packs, sizeof(buf_udp_packs), 0, (struct sockaddr *) &si_other, slen) == -1 )
		{
			fprintf(stderr, "sendto()");
			exit(1);
		}

		std::this_thread::sleep_for(50ms);
	}

}
