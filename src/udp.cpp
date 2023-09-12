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

#include "udp.hpp"

#include "string.h"

#define UDP_BUFLEN 255

using namespace std;

udp_package udp_packs[2];

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

void udp_func()
{

	std::cout << "UPD thread started!\n";

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		std::cerr << "Error creating socket" << std::endl;
		exit(1);
	}

	// Задаем адрес сервера
	struct sockaddr_in serveraddr{};
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(config.UDP_PORT);
	serveraddr.sin_addr.s_addr = INADDR_ANY;

	// Привязываем сокет к адресу сервера
	if (bind(sockfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) < 0) {
		std::cerr << "Error binding socket" << std::endl;
		close(sockfd);
		exit(1);
	}

	std::cout << "UDP thread entered infinity loop.\n";

	// Бесконечный цикл приема сообщений
	while (!restart_threads || !kill_threads) {

		char buffer[UDP_BUFLEN] = {0};
		struct sockaddr_in clientaddr{};
		socklen_t clientaddrlen = sizeof(clientaddr);

		// Принимаем сообщение
		ssize_t bytesRead = recvfrom(sockfd, buffer, sizeof(buffer)-1, 0, (struct sockaddr*) &clientaddr, &clientaddrlen);
		if (bytesRead < 0) {
			std::cerr << "Error receiving message" << std::endl;
			break;
		}

		// Выводим полученное сообщение
		std::cout << "Received message: " << buffer << std::endl;

//		// Отправляем ответное сообщение обратно клиенту
//		std::string response = "Hello, client!";
//		ssize_t bytesSent = sendto(sockfd, response.c_str(), response.length(), 0, (struct sockaddr*) &clientaddr, clientaddrlen);
//		if (bytesSent < 0) {
//			std::cerr << "Error sending response" << std::endl;
//			break;
//		}
	}

	std::cout << "UDP out of infinity loop.\n";

	// Закрываем сокет
	close(sockfd);

//	struct sockaddr_in si_other;
//	int sock, slen = sizeof(si_other);
//
//	if ( (sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 )
//	{
//		fprintf(stderr, "socket");
//		exit(1);
//	}
//
//	memset((char *) &si_other, 0, sizeof(si_other));
//	si_other.sin_family = AF_INET;
//	si_other.sin_port = htons(config.UDP_PORT);
//
//	char udp_addr[15];
//	strcpy(udp_addr, config.UDP_ADDR.c_str());
//	if (inet_aton(udp_addr, &si_other.sin_addr) == 0)
//	{
//		fprintf(stderr, "inet_aton() failed\n");
//		exit(1);
//	}
//
//	std::cout << "UDP thread entered infinity loop.\n";
//
//	uint16_t counter = 0;
//
//	while (!restart_threads || !kill_threads) {
//
//		counter++;
//
//		parse_results_mtx.lock();
//		for (int i = 0; i < 2; i++)
//			make_udp_pack(counter, udp_packs[i], parse_results[i]);
//		parse_results_mtx.unlock();
//
//		size_t sz = sizeof(udp_packs);
//		if (UDP_LOG) {
//			char* my_s_bytes = reinterpret_cast<char*>(&udp_packs);
//			for (size_t i = 0; i < sz; i++)
//				printf("%02x ", my_s_bytes[i]);
//			printf("\n");
//		}
//
//		if ( sendto(sock, &udp_packs, sizeof(udp_packs), 0, (struct sockaddr *) &si_other, slen) == -1 )
//		{
//			fprintf(stderr, "sendto()");
//			exit(1);
//		}
//
//		std::this_thread::sleep_for(50ms);
//	}

}
