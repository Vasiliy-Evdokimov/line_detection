/*
 * shared_memory.cpp
 *
 *  Created on: 12 сент. 2023 г.
 *      Author: vevdokimov
 */

#include <iostream>
#include <cstring>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#include "config.hpp"
#include "shared_memory.hpp"

using namespace std;

int config_sm_id;
ConfigData* config_sm_ptr;

int init_shared_memory()
{

	//	генерация ключа для shared memory
	key_t config_key = ftok(SM_NAME, CONFIG_SM_ID);
	if (config_key == -1) {
		perror("ftok");
		return 1;
	}

	//	создание shared memory
	config_sm_id = shmget(config_key, sizeof(ConfigData), IPC_CREAT | 0666);
	if (config_sm_id == -1) {
		perror("shmget");
		return 1;
	}

	//	присоединение к shared memory
	config_sm_ptr = (ConfigData*) shmat(config_sm_id, nullptr, SHM_R | SHM_W);
	if (config_sm_ptr == (void*) -1) {
		perror("shmat");
		shmctl(config_sm_id, IPC_RMID, nullptr);
		return 1;
	}

	return 0;

}

int write_config_sm(ConfigData& aConfig) {

	if (config_sm_ptr == (void*) -1) return 1;
	memcpy(config_sm_ptr, &aConfig, sizeof(aConfig));
	return 0;

}

int read_config_sm(ConfigData& aConfig) {

	if (config_sm_ptr == (void*) -1) return 1;
	memcpy(&aConfig, config_sm_ptr, sizeof(aConfig));
	return 0;

}

int init_config_sm(ConfigData& aConfig) {

	shmid_ds state;
	shmctl(config_sm_id, IPC_STAT, &state);
	//
	cout << "state.shm_nattch = " << state.shm_nattch << endl;
	//	если количество подключенных к SM = 1, т.е. только это приложение
	if (state.shm_nattch == 1) {
		//	то записываем в SM данные из конфига прочитанного при инициализации
		write_config_sm(aConfig);
	} else {	//	если подключенных > 1
		int pid = getpid();
		//	если PID в SM совпадает с PID программы
		if (config_sm_ptr->PID == pid) {	//	то это перезапуск потоков
			//	читаем конфиг из SM
			read_config_sm(aConfig);
		} else {							//	то это запуск программы при запущенном вэб-сервере
			//	записываем в SM конфиг с PID программы
			write_config_sm(aConfig);
		}
	}

	return 0;

}

