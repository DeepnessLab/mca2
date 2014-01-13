/*
 * MulticoreManager.h
 *
 *  Created on: Dec 19, 2011
 *      Author: yotam
 */

#ifndef MULTICOREMANAGER_H_
#define MULTICOREMANAGER_H_

#include <pthread.h>
#include "../DumpReader/BoundedBuffer/Scanner.h"
#include "CyclicLockFreeQueue.h"
#include "HeavyPacket.h"
#include "../Common/GlobalTimer.h"

/*
#ifdef ALERT_ALWAYS
#define ALERT_MODE_WORK_GROUP_USAGE_HEAVY_RATE_THRESHOLDS { 0,0,0,0 }
#else
#ifdef ALERT_NEVER
#define ALERT_MODE_WORK_GROUP_USAGE_HEAVY_RATE_THRESHOLDS { 1,1,1,1 }
#else
#define ALERT_MODE_WORK_GROUP_USAGE_HEAVY_RATE_THRESHOLDS { 0.00, 0.1, 0.5, 0.8}
#endif
#endif
*/

#define NUM_OF_THRESHOLDS 4
#define HEAVY_PACKET_QUEUE_SIZE 1310720
#define MANAGER_CHECK_INTERVAL_REGULAR_MODE 1 // in seconds
#define MANAGER_CHECK_INTERVAL_ALERT_MODE 1 // in seconds
//#define SCANNER_WAIT_WHEN_NO_PACKETS 5 // in seconds
//#define ALERT_MODE_RECHECK

#define SCANNER_NUM_PACKETS_TO_UPDATE_MANAGER 10

#define DEFAULT_WORK_GROUP_SIZE 2
#define DEFAULT_MAX_DEDICATED_WORK_GROUPS 1

//#define LOG_MULTICORE_MANAGER

//#define MAX_SCANNERS 64

enum SYSTEM_MODE {
	SYSTEM_MODE_REGULAR,
	SYSTEM_MODE_ALERT
};

typedef struct multicore_manager {
	unsigned int num_scanners;
	unsigned int work_group_size;
	unsigned int max_dedicated_work_groups;
	double *work_groups_thresholds;
	unsigned int *total_count;
	unsigned int *heavy_count;
	enum SYSTEM_MODE system_mode;
	struct scanner_data **scanners;
	CyclicLockFreeQueue **heavy_packet_queues;
	unsigned int num_of_regular_threads;
	unsigned int num_of_dedicated_threads;
	int *next_target;//[MAX_SCANNERS];
	pthread_t thread;
	int stopped;
	GlobalTimer gtimer;
	int scanners_done;
	int packets_to_steal;
	int dedicated_use_compressed;
	Timer alert_mode_timer;
	int alert_mode_used;
} MulticoreManager;

void multicore_manager_init(MulticoreManager *manager, ScannerData *scanners, unsigned int num_scanners,
		unsigned int work_group_size, unsigned int max_dedicated_work_groups,
		int packets_to_steal, int dedicated_use_compressed);

void multicore_manager_set_thresholds(MulticoreManager *manager, double *thresholds);

void multicore_manager_update(MulticoreManager *manager, int thread_id, unsigned int num_heavy, unsigned int num_total);

void multicore_manager_start(MulticoreManager *manager);

void multicore_manager_stop(MulticoreManager *manager);

void multicore_manager_stop_all(MulticoreManager *manager);

void multicore_manager_join(MulticoreManager *manager);

void multicore_manager_destroy(MulticoreManager *manager);

int multicore_manager_transfer_packet(MulticoreManager *manager, HeavyPacket *hp, unsigned int from_thread);

void multicore_manager_scanner_done(MulticoreManager *manager);

#endif /* MULTICOREMANAGER_H_ */
