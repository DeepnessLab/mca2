/*
 * MulticoreManager.c
 *
 *  Created on: Dec 19, 2011
 *      Author: yotam
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "MulticoreManager.h"
#include "HeavyPacket.h"

inline double get_heavy_packet_rate(MulticoreManager *manager) {
	unsigned int sum_heavy, sum_total, i;
	sum_heavy = sum_total = 0;
	for (i = 0; i < manager->num_scanners; i++) {
#ifdef __GNUC__
		sum_heavy += __sync_lock_test_and_set(&(manager->heavy_count[i]), 0);
		sum_total += __sync_lock_test_and_set(&(manager->total_count[i]), 0);
#else
		sum_heavy += manager->heavy_count[i];
		sum_total += manager->total_count[i];
#endif
	}
	if (sum_total == 0)
		return 0;
	//printf("heavy: %u, total: %u\n", sum_heavy, sum_total);
	return ((double)sum_heavy) / sum_total;
}

void handle_alert_mode(MulticoreManager *manager, double rate) { //, const double *thresholds) {
	unsigned int i, j, changed;
	unsigned int dedicated_wgs;
	int num_dedicated_threads, num_regular_threads;//, num_queues;
	CyclicLockFreeQueue *queue;
	int queue_not_empty;
	int size;
#ifdef ALERT_MODE_RECHECK
	int exit_alert_mode;
#endif
	int status;

	dedicated_wgs = 0;
	changed = 0;
	num_dedicated_threads = 0;
#ifdef ALERT_MODE_RECHECK
	exit_alert_mode = 0;
#endif

	manager->alert_mode_used = 1;
	startTiming(&(manager->alert_mode_timer));

	num_regular_threads = manager->num_of_regular_threads;

#ifdef GLOBAL_TIMING
	global_timer_set_event(&(manager->gtimer), "Alert mode started", "MulticoreManager");
#endif

#ifdef ALERT_MODE_RECHECK
	do {
#endif

		// Find number of cores to use according to current rate and given thresholds
		for (i = 0; i < manager->max_dedicated_work_groups; i++) {
			if (manager->work_groups_thresholds[manager->max_dedicated_work_groups - i - 1] < rate) {
				// Do not reduce number of dedicated threads after assigning them
				if (dedicated_wgs < manager->max_dedicated_work_groups - i) {
					dedicated_wgs = manager->max_dedicated_work_groups - i;
					changed = 1;
				}
				break;
			}
		}

		if (changed) {
#ifdef DROP_ALL_HEAVY
			num_dedicated_threads = 0;
			num_regular_threads = manager->num_scanners;
#else
			num_dedicated_threads = dedicated_wgs * manager->work_group_size;
			num_regular_threads = manager->num_scanners - num_dedicated_threads;
#endif

			if (num_regular_threads > 0 && num_dedicated_threads > 0) {
				// Tells each regular thread what is its next target dedicated thread to pass heavy packets to
				for (i = 0; i < num_regular_threads; i++) {
					manager->next_target[i + num_dedicated_threads] = (i % num_dedicated_threads);
				}
			}

#ifdef LOG_MULTICORE_MANAGER
	printf("MulticoreManager: ALERT MODE: %d dedicated threads are used\n", num_dedicated_threads);
#endif

			// Initialize queues
			//num_queues = num_dedicated_threads * manager->num_scanners; // must be num_scanners and not num_regular because of array ordering
			for (i = 0; i < num_dedicated_threads; i++) {
				for (j = 0; j < num_regular_threads; j++) {
					queue = (CyclicLockFreeQueue*)malloc(sizeof(CyclicLockFreeQueue));
					cyclic_lock_free_queue_init(queue, HEAVY_PACKET_QUEUE_SIZE);
					GET_HEAVY_PACKET_QUEUE(manager, j + num_dedicated_threads, i) = queue;
				}
			}
			/*
			for (i = 0; i < num_queues; i++) {
				if (manager->heavy_packet_queues[i] == NULL) {
					queue = (CyclicLockFreeQueue*)malloc(sizeof(CyclicLockFreeQueue));
					cyclic_lock_free_queue_init(queue, HEAVY_PACKET_QUEUE_SIZE);
					manager->heavy_packet_queues[i] = queue;
				}
			}
			*/
			manager->num_of_regular_threads = num_regular_threads;
			manager->num_of_dedicated_threads = num_dedicated_threads;

			// Change scanners machine type (only after initializing queues)
			for (i = 0; i < manager->num_scanners; i++) {
				// This code also supports reducing number of dedicated threads, but
				// it will not happen as long as this is not allowed above
				if (i < num_dedicated_threads) {
					if (num_regular_threads > 0) {
						list_set_lock(manager->scanners[i]->packet_queue, 1);
					} else {
						list_set_lock(manager->scanners[i]->packet_queue, 0);
					}
					scanner_set_machine_type(manager->scanners[i], MACHINE_TYPE_COMPRESSED);
				} else {
					list_set_lock(manager->scanners[i]->packet_queue, 0);
					scanner_set_machine_type(manager->scanners[i], MACHINE_TYPE_NAIVE);
				}
				//printf("Scanner %u bytes scanned before alert: %lu\n", i, manager->scanners[i]->bytes_scanned);
				scanner_set_alert_mode(manager->scanners[i], 1);
			}
		}
#ifndef ALERT_MODE_RECHECK
	do{
#endif
		// Sleep some seconds
		sleep(MANAGER_CHECK_INTERVAL_ALERT_MODE);
		rate = get_heavy_packet_rate(manager);
#ifdef LOG_MULTICORE_MANAGER
	printf("MulticoreManager: ALERT MODE: Current heavy packet rate is %1.4f\n", rate);
#endif

#ifdef GLOBAL_TIMING
#ifdef EXTRA_EVENTS
		global_timer_set_event(&(manager->gtimer), "Manager checks current state (alert mode)", "MulticoreManager");
#endif
#endif


		if (manager->scanners_done == manager->num_of_regular_threads) {
			for (i = 0; i < manager->num_of_dedicated_threads; i++) {
				manager->scanners[i]->done = 1;
			}
			//manager->stopped = 1;
		}

#ifdef ALERT_MODE_RECHECK
		if (rate < manager->work_groups_thresholds[0]) {
			exit_alert_mode = 1;
		}
#endif

		queue_not_empty = 0;
		for (i = num_dedicated_threads; i < manager->num_scanners && !queue_not_empty; i++) {
			if (!list_is_empty(manager->scanners[i]->packet_queue)) {
				queue_not_empty = 1;
			}
		}
		// STOPPING ALL THREADS WHEN REGULAR THREADS' INPUT QUEUES GET EMPTY
		if (!queue_not_empty) {
#ifdef GLOBAL_TIMING
			global_timer_set_event(&(manager->gtimer), "Stopping scanners", "MulticoreManager");
#endif
			manager->stopped = 1;
			for (i = 0; i < manager->num_scanners; i++) {
				manager->scanners[i]->done = 1;
			}
			endTiming(&(manager->alert_mode_timer));
		}


		status = 0;
		for (i = 0; i < num_dedicated_threads; i++) {
			for (j = 0; j < num_regular_threads; j++) {
				queue = GET_HEAVY_PACKET_QUEUE(manager, j + num_dedicated_threads, i);
				if (queue) {
					if (cyclic_lock_free_queue_isempty(queue) == CYCLIC_PACKET_QUEUE_SUCEESS) {
						// There are still heavy packets to process
						status = 1;
					}
				}
			}
		}


#ifdef ALERT_MODE_RECHECK
	} while (!(manager->stopped) && !exit_alert_mode);// && rate > thresholds[0]);
#else
	} while (!(manager->stopped));
#endif

	if (!manager->stopped) {
		// Return scanners to regular mode
		for (i = 0; i < manager->num_scanners; i++) {
			scanner_set_alert_mode(manager->scanners[i], 0);
			scanner_set_machine_type(manager->scanners[i], MACHINE_TYPE_NAIVE);
		}
	}


	if (manager->next_target != NULL) {
		free(manager->next_target);
		manager->next_target = NULL;
	}

#ifdef GLOBAL_TIMING
	global_timer_set_event(&(manager->gtimer), "Manager Stopped", "MulticoreManager");
#endif

	for (i = 0; i < num_dedicated_threads; i++) {
		for (j = 0; j < num_regular_threads; j++) {
			queue = GET_HEAVY_PACKET_QUEUE(manager, j + num_dedicated_threads, i);
			if (queue) {

#ifdef LOCK_CYCLIC_QUEUE
				size = cyclic_lock_free_queue_get_size(queue);
#else
				HeavyPacket hp;
				//int status;
				size = 0;
				while ((status = cyclic_lock_free_queue_dequeue(queue, &hp)) == CYCLIC_PACKET_QUEUE_SUCEESS) {
					size++;
				}
#endif
				if (size > 0) {
					printf("Queue of thread %d to %d is not empty (size: %d)\n", j + num_dedicated_threads, i, size);
				} else {
#ifdef LOCK_CYCLIC_QUEUE
					printf("Size of queue of thread %d to %d: %d (in: %d, out: %d)\n", j + num_dedicated_threads, i, size, queue->in, queue->out);
#else
					//printf("Size of queue of thread %d to %d: %d\n", j + num_dedicated_threads, i, size);
#endif
				}
				cyclic_lock_free_queue_destroy(queue);
				free(queue);
			}
		}
	}
}

void *start_multicore_manager_thread(void *param) {
	MulticoreManager *manager;
	double rate;

	//const double WORK_GROUP_USAGE_HEAVY_RATE_THRESHOLDS[] = ALERT_MODE_WORK_GROUP_USAGE_HEAVY_RATE_THRESHOLDS;

	manager = (MulticoreManager*)param;

#ifdef LOG_MULTICORE_MANAGER
	printf("MulticoreManager: Started\n");
#endif

#ifdef GLOBAL_TIMING
	global_timer_set_event(&(manager->gtimer), "Manager Started", "MulticoreManager");
#endif

	while (!(manager->stopped)) {
		if (manager->work_groups_thresholds[0] > 0) {
			sleep(MANAGER_CHECK_INTERVAL_REGULAR_MODE);

			// Check current mode
			rate = get_heavy_packet_rate(manager);
		} else {
			rate = 1;
		}
#ifdef LOG_MULTICORE_MANAGER
	printf("MulticoreManager: Current heavy packet rate is %1.4f\n", rate);
#endif

#ifdef GLOBAL_TIMING
#ifdef EXTRA_EVENTS
		global_timer_set_event(&(manager->gtimer), "Manager checks current state (regular mode)", "MulticoreManager");
#endif
#endif

		if (rate > manager->work_groups_thresholds[0]) {
#ifdef LOG_MULTICORE_MANAGER
	printf("MulticoreManager: Entering ALERT MODE\n");
#endif
			// ALERT MODE STARTING
			handle_alert_mode(manager, rate);
		}

		if (manager->scanners_done == manager->num_scanners) {
			manager->stopped = 1;
		}

	}
#ifdef LOG_MULTICORE_MANAGER
	printf("MulticoreManager: Stopped\n");
#endif
	pthread_exit(NULL);
}

void multicore_manager_stop_all(MulticoreManager *manager) {
	int i;
#ifdef GLOBAL_TIMING
	global_timer_set_event(&(manager->gtimer), "Stopping scanners", "MulticoreManager");
#endif
	manager->stopped = 1;
	for (i = 0; i < manager->num_scanners; i++) {
		manager->scanners[i]->done = 1;
	}
	endTiming(&(manager->alert_mode_timer));
}

void multicore_manager_init(MulticoreManager *manager, ScannerData *scanners, unsigned int num_scanners, unsigned int work_group_size, unsigned int max_dedicated_work_groups, int packets_to_steal, int dedicated_use_compressed) {
	int i;
	manager->num_scanners = num_scanners;
	manager->work_group_size = work_group_size;
	manager->max_dedicated_work_groups = max_dedicated_work_groups;
	manager->total_count = (unsigned int*)malloc(sizeof(unsigned int) * num_scanners);
	manager->heavy_count = (unsigned int*)malloc(sizeof(unsigned int) * num_scanners);
	manager->system_mode = SYSTEM_MODE_REGULAR;
	manager->scanners = (ScannerData**)malloc(sizeof(ScannerData*) * num_scanners);
	manager->next_target = (int*)malloc(sizeof(int) * num_scanners);//NULL;
	manager->stopped = FALSE;
	manager->num_of_regular_threads = num_scanners;
	manager->num_of_dedicated_threads = 0;
	manager->scanners_done = 0;
	manager->packets_to_steal = packets_to_steal;
	manager->dedicated_use_compressed = dedicated_use_compressed;
	manager->alert_mode_used = 0;

	memset(manager->heavy_count, 0, sizeof(unsigned int) * num_scanners);
	memset(manager->total_count, 0, sizeof(unsigned int) * num_scanners);

	for (i = 0; i < num_scanners; i++) {
		manager->scanners[i] = &(scanners[i]);
	}

#ifdef GLOBAL_TIMING
	global_timer_init(&(manager->gtimer), num_scanners);
#endif

	// A matrix of pointers to queues
	// Dimensions: <num of threads> ^ 2
	// Rows: regular threads (producers)
	// Cols: dedicated threads (consumers)
	// Access the queue for producer j and consumer i: [i * num_threads + j]
	// Use: GET_HEAVY_PACKET_QUEUE macro for that!
	manager->heavy_packet_queues = (CyclicLockFreeQueue**)malloc(sizeof(CyclicLockFreeQueue*) * num_scanners * num_scanners);
	memset(manager->heavy_packet_queues, 0, sizeof(CyclicLockFreeQueue*) * num_scanners * num_scanners);
}

void multicore_manager_set_thresholds(MulticoreManager *manager, double *thresholds) {
	manager->work_groups_thresholds = thresholds;
}

void multicore_manager_update(MulticoreManager *manager, int thread_id, unsigned int num_heavy, unsigned int num_total) {
	// Updating total first, so if meanwhile manager is reading a mistake - it will give a lower percentage
	__sync_fetch_and_add(&(manager->total_count[thread_id]), num_total);
	__sync_fetch_and_add(&(manager->heavy_count[thread_id]), num_heavy);
}

void multicore_manager_start(MulticoreManager *manager) {
	pthread_create(&(manager->thread), NULL, start_multicore_manager_thread, manager);
}

void multicore_manager_stop(MulticoreManager *manager) {
	manager->stopped = TRUE;
}

void multicore_manager_join(MulticoreManager *manager) {
	pthread_join(manager->thread, NULL);
}

void multicore_manager_destroy(MulticoreManager *manager) {
	free(manager->total_count);
	free(manager->heavy_count);
	free(manager->scanners);
	free(manager->heavy_packet_queues);
	free(manager->next_target);
}

int multicore_manager_transfer_packet(MulticoreManager *manager, HeavyPacket *hp, unsigned int from_thread) {
	CyclicLockFreeQueue *q;
	unsigned int to_thread;
	int i;

	i = from_thread;// - manager->num_of_dedicated_threads;
	to_thread = manager->next_target[i];
	manager->next_target[i] = (manager->next_target[i] + (manager->num_of_regular_threads)) % (manager->num_of_dedicated_threads);

#ifdef LOG_MULTICORE_MANAGER
	printf(">> MulticoreManager: Transferring heavy packet from thread %u to thread %u\n", from_thread, to_thread);
#endif

	q = GET_HEAVY_PACKET_QUEUE(manager, from_thread, to_thread);
	cyclic_lock_free_queue_enqueue(q, hp);

	return to_thread;
}

void multicore_manager_scanner_done(MulticoreManager *manager) {
#ifdef __GNUC__
	__sync_fetch_and_add(&(manager->scanners_done), 1);
#else
	manager->scanners_done++;
#endif
}
