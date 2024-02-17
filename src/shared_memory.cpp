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

#include <mutex>

#include "common_types.hpp"
#include "config.hpp"
#include "log.hpp"
#include "shared_memory.hpp"

using namespace std;

mutex config_sm_mtx;	//	мьютекс чтения/записи конфига
mutex results_sm_mtx;	//	мьютекс чтения/записи результатов

int config_sm_id;
ConfigData* config_sm_ptr;
int results_sm_id[CAM_COUNT];
ResultFixed* results_sm_ptr[CAM_COUNT];

int init_shared_memory()
{
	//	ipcs -m			- список сегментов shared memory
	//	ipcrm -m <ID> 	- удаление указанного сегмента shared memory

	std::string err_prefix = "config_sm error = ";

	key_t config_key = ftok(SM_NAME, CONFIG_SM_ID);
	if (config_key == -1)
	{
		write_log(err_prefix + "ftok");
		perror("ftok");
		return 1;
	}

	config_sm_id = shmget(config_key, sizeof(ConfigData), IPC_CREAT | 0666);
	if (config_sm_id == -1)
	{
		write_log(err_prefix + "shmget");
		perror("shmget");
		return 1;
	}

	config_sm_ptr = (ConfigData*) shmat(config_sm_id, nullptr, SHM_R | SHM_W);
	if (config_sm_ptr == (void*) -1)
	{
		write_log(err_prefix + "shmat");
		perror("shmat");
		shmctl(config_sm_id, IPC_RMID, nullptr);
		return 1;
	}

	//

	key_t results_key[CAM_COUNT];
	for (int i = 0; i < CAM_COUNT; i++)
	{
		err_prefix = "results_sm[" + to_string(i) + "] error = ";

		results_key[i] = ftok(SM_NAME, CAMRES_SM_ID + i);
		if (results_key[i] == -1)
		{
			write_log(err_prefix + "ftok");
			perror("ftok");
			return 1;
		}

		results_sm_id[i] = shmget(results_key[i], sizeof(ResultFixed), IPC_CREAT | 0666);
		if (results_sm_id[i] == -1)
		{
			write_log(err_prefix + "shmget");
			perror("shmget");
			return 1;
		}

		results_sm_ptr[i] = (ResultFixed*) shmat(results_sm_id[i], nullptr, SHM_R | SHM_W);
		if (results_sm_ptr[i] == (void*) -1)
		{
			write_log(err_prefix + "shmat");
			perror("shmat");
			shmctl(results_sm_id[i], IPC_RMID, nullptr);
			return 1;
		}
	}

	return 0;
}

int write_config_sm(ConfigData& aConfig)
{
	if (config_sm_ptr == (void*) -1) return 1;
	//
	config_sm_mtx.lock();
	memcpy(config_sm_ptr, &aConfig, sizeof(aConfig));
	config_sm_mtx.unlock();
	//
	return 0;
}

int read_config_sm(ConfigData& aConfig)
{
	if (config_sm_ptr == (void*) -1) return 1;
	//
	config_sm_mtx.lock();
	memcpy(&aConfig, config_sm_ptr, sizeof(aConfig));
	config_sm_mtx.unlock();
	//
	return 0;
}

int init_config_sm(ConfigData& aConfig)
{
	shmid_ds state;
	shmctl(config_sm_id, IPC_STAT, &state);
	//
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
	//
	return 0;
}

int write_results_sm(ResultFixed& aResult, int aIndex)
{
	if (results_sm_ptr[aIndex] == (void*) -1) return 1;
	//
	high_resolution_clock::time_point result_time_point = high_resolution_clock::now();
	memcpy(aResult.result_time_point, &result_time_point, RESULT_TIME_SIZE);
	//
	results_sm_mtx.lock();
	memcpy(results_sm_ptr[aIndex], &aResult, sizeof(aResult));
	results_sm_mtx.unlock();
	//
	return 0;
}

int read_results_sm(ResultFixed& aResult, int aIndex)
{
	if (results_sm_ptr == (void*) -1) return 1;
	//
	results_sm_mtx.lock();
	memcpy(&aResult, results_sm_ptr[aIndex], sizeof(aResult));
	results_sm_mtx.unlock();
	//
	high_resolution_clock::time_point result_time_point;
	memcpy(&result_time_point, aResult.result_time_point, RESULT_TIME_SIZE);
	//
	high_resolution_clock::time_point now = high_resolution_clock::now();
	//
	auto duration = duration_cast<std::chrono::milliseconds>(now - result_time_point);
	aResult.error_flags |= (((duration.count() > config.CAM_TIMEOUT) ? 1 : 0) << 3);
	//
	return 0;
}

void parse_result_to_sm(ParseImageResult& parse_result, int aIndex)
{
	ResultFixed rfx = parse_result.ToFixed();
	write_results_sm(rfx, aIndex);
}
