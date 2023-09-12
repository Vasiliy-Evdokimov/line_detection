/*
 * shared_memory.cpp
 *
 *  Created on: 12 сент. 2023 г.
 *      Author: vevdokimov
 */

#include <cstring>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#include "config.hpp"
#include "shared_memory.hpp"

using namespace std;

int init_shared_memory()
{

	//	генерация ключа для shared memory
	key_t key = ftok(SM_NAME, SM_ID);
	if (key == -1) {
		perror("ftok");
		return 1;
	}

	//	создание shared memory
	int mem_id = shmget(key, sizeof(ConfigData), IPC_CREAT | 0666);
	if (mem_id == -1) {
		perror("shmget");
		return 1;
	}

	// Присоединение к shared memory
	ConfigData* ptr = (ConfigData*) shmat(mem_id, nullptr, SHM_R | SHM_W);
	if (ptr == (void*) -1) {
		perror("shmat");
		shmctl(mem_id, IPC_RMID, nullptr);
		return 1;
	}

	shmid_ds state;
	shmctl(mem_id, IPC_STAT, &state);
	if (state.shm_nattch == 1) {
		memcpy(ptr, &config, sizeof(config));
	} else {
		memcpy(&config, ptr, sizeof(config));
	}

	return 0;

}

