/*
 * Implementation of a scanner
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "../DumpReader.h"
#include "Scanner.h"
#include "../../StateMachine/TableStateMachine.h"
#include "../../Multicore/MulticoreManager.h"
#ifdef PAPI
#include <papi.h>
#endif
//#define PACKET_READ_BUFFER_SIZE 50

#define NUM_OF_FINISH_ROUNDS 2

#define UNCOMMON_HISTOGRAM_SIZE 101

const unsigned int MASKS[] = { 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF };

void printIP(unsigned int ip) {
	printf("%u.%u.%u.%u", ((ip & MASKS[0]) >> 24), ((ip & MASKS[1]) >> 16), ((ip & MASKS[2]) >> 8), ip & MASKS[3]);
}

void *start_scanner_thread(void *param) {
	int status;
	int is_heavy, match_table_result, last_idx_in_root;
	ScannerData *scanner;
	Packet *packet;
	HeavyPacket hp;
	CyclicLockFreeQueue *heavy_queue;
	int next_writer_id, last_writer_id;
	unsigned int num_heavy, num_total;
	int free_packet;
	int dest_thread;
	Packet *next_packet;
	int i;
	int finish_round;
	double uncommonRate;
	int retval;
#ifdef DONT_TRANSFER_STOLEN
	int dont_transfer_next;
#endif

#ifdef ISOLATION_TEST
	int total_heavy, total_light, false_neg, false_pos;
#endif
#ifdef PAPI
	int event_set;
	long long *papi_values;
	int papi_ok;
#endif

#ifdef SCANNER_STATISTICS
	int packets_read_since_alert;
	int packets_taken_from_dedicated;

	int num_heavy_since_alert;
	int num_regular_since_alert;
	int num_transferred_since_alert;
	int num_received_since_alert;
	int num_failed_dequeue_since_alert;

	int uncommonRateHistogram[UNCOMMON_HISTOGRAM_SIZE];
	//char uncommonHistPrintBuff[1024];
	//int len;
	//double sumOfHpRates;
	//long packetsCounted;
#endif
#ifdef DROP_STATISTICS
	int dropped;
#endif

#ifdef GLOBAL_TIMING
	unsigned int bytes_scanned_recently;
#endif
#ifdef EXTRA_EVENTS
	int first_packet_taken_from_dedicated;
	int my_queue_is_empty;
	int dedicated_stopped;
	int first_packet_taken_from_cyclic;
#endif

	//Packet *packets[PACKET_READ_BUFFER_SIZE];
	//int first, last, lastInsLocation;
	//int count;
#ifdef COUNT_BY_DEPTH
	// Histogram of accesses by depth
	// Histogram of accesses by state

	long numAccesses, idx;

	long accessesByDepth[200];
	long *accessesByState;
#endif
#ifdef PRINT_STATE_VISIT_HIST
	int idx;
	int *visits;
#endif
	scanner = (ScannerData*)param;

#ifdef COUNT_BY_DEPTH
	accessesByState = (long*)malloc(sizeof(long) * ((TableStateMachine*)(scanner->machine))->numStates);

	memset(accessesByState, 0, sizeof(long) * ((TableStateMachine*)(scanner->machine))->numStates);
	memset(accessesByDepth, 0, sizeof(long) * 200);

	numAccesses = 0;

#endif

#ifdef PRINT_STATE_VISIT_HIST
	visits = (int*)malloc(sizeof(int) * scanner->tableMachine->numStates);
	memset(visits, 0, sizeof(int) * scanner->tableMachine->numStates);
#endif

#ifdef ISOLATION_TEST
	total_heavy = total_light = false_neg = false_pos = 0;
#endif

#ifdef DONT_TRANSFER_STOLEN
	dont_transfer_next = 0;
#endif

	/*
	memset(packets, 0, sizeof(Packet*) * PACKET_READ_BUFFER_SIZE);
	first = 0; last = 0;
	count = 0;
	*/
	scanner->bytes_scanned = 0;
	scanner->bytes_scanned_since_alert = 0;
#ifdef GLOBAL_TIMING
	bytes_scanned_recently = 0;
#endif
#ifdef EXTRA_EVENTS
	first_packet_taken_from_dedicated = 0;
	my_queue_is_empty = 0;
	dedicated_stopped = 0;
	first_packet_taken_from_cyclic = 0;
#endif
	startTiming(&(scanner->timer));

#ifdef SCANNER_STATISTICS
	packets_read_since_alert = packets_taken_from_dedicated = 0;
	num_heavy_since_alert = num_regular_since_alert = num_transferred_since_alert = num_received_since_alert = num_failed_dequeue_since_alert = 0;

	memset(uncommonRateHistogram, 0, sizeof(int) * UNCOMMON_HISTOGRAM_SIZE);
	//sumOfHpRates = 0;
	//packetsCounted = 0;
#endif
#ifdef DROP_STATISTICS
	dropped = 0;
#endif

	//packet = NULL;
	num_total = num_heavy = 0;
	finish_round = 0;
	next_writer_id = -1; //scanner->scanner_id % scanner->manager->num_of_regular_threads + scanner->manager->num_of_dedicated_threads;

#ifdef PAPI
	papi_ok = 1;
	event_set = PAPI_NULL;
	//event_set, prv, num_events;
	//long long *papi_values;
	papi_values = (long long*)malloc(sizeof(long long) * 3);

	if (PAPI_register_thread() != PAPI_OK) {
		fprintf(stderr, "Cannot register thread %d with PAPI\n", scanner->scanner_id);
		papi_ok = 0;
	}
	if (papi_ok && PAPI_create_eventset(&event_set) != PAPI_OK) {
		fprintf(stderr, "Cannot create PAPI event set for thread %d\n", scanner->scanner_id);
		papi_ok = 0;
	}
	if (papi_ok && (
			PAPI_add_event(event_set, PAPI_L1_DCM) != PAPI_OK ||
			PAPI_add_event(event_set, PAPI_L2_DCM) != PAPI_OK ||
			PAPI_add_event(event_set, PAPI_L3_TCM) != PAPI_OK)) {
		fprintf(stderr, "Cannot add PAPI events for thread %d\n", scanner->scanner_id);
		papi_ok = 0;
	}
	if (papi_ok && PAPI_start(event_set) != PAPI_OK) {
		fprintf(stderr, "Cannot start PAPI for thread %d\n", scanner->scanner_id);
		papi_ok = 0;
	}
#endif

	while (!scanner->done) {
		last_idx_in_root = -1;
		free_packet = 1;

		hp.packet = NULL;
		hp.last_idx_in_root = 0;
		if ((!scanner->finishing_alert_mode) && (scanner->isTableMachine || scanner->manager->num_of_regular_threads == 0)) {
			// Retrieve packets from regular queue if thread is regular or if all threads are dedicated
			packet = (Packet*)list_dequeue(scanner->packet_queue, &status);
			if (status == LIST_STATUS_DEQUEUE_EMPTY) {
				multicore_manager_stop_all(scanner->manager);
			}
#ifdef SCANNER_STATISTICS
			if (!status && scanner->alert_mode_active)
				packets_read_since_alert++;
#endif
		} else {
			status = -999;
			if (next_writer_id == -1) {
				//next_writer_id = scanner->scanner_id % scanner->manager->num_of_regular_threads + scanner->manager->num_of_dedicated_threads;
				next_writer_id = scanner->manager->num_of_dedicated_threads;
			}
			last_writer_id = next_writer_id;
			do {
				// This thread runs in "Dedicated" mode and there are also regular threads in system
				heavy_queue = GET_HEAVY_PACKET_QUEUE(scanner->manager, next_writer_id, scanner->scanner_id);
				next_writer_id = (next_writer_id + 1);
				if (next_writer_id >= scanner->manager->num_scanners) {
					next_writer_id = scanner->manager->num_of_dedicated_threads;
				}

				//printf("ID: %d, Writer: %d, Next: %d\n", scanner->scanner_id, last_writer_id, next_writer_id);

				hp.last_idx_in_root = 0;
				status = cyclic_lock_free_queue_dequeue(heavy_queue, (void*)(&hp));
				if (status == CYCLIC_PACKET_QUEUE_SUCEESS) {// && hp != NULL) {
					status = LIST_STATUS_DEQUEUE_SUCCESS;
					packet = hp.packet;
					last_idx_in_root = hp.last_idx_in_root;

//					if (hp.last_idx_in_root < 0 || hp.last_idx_in_root > 10000) {
//						fprintf(stderr, "Warning: weird value for hp.last_index_in_root (1): %d\n", hp.last_idx_in_root);
//					}

#ifdef SCANNER_STATISTICS
					num_received_since_alert++;
#endif
#ifdef GLOBAL_TIMING
#ifdef EXTRA_EVENTS
					if (!first_packet_taken_from_cyclic) {
						first_packet_taken_from_cyclic = 1;
						global_timer_set_event(&(scanner->manager->gtimer), "Dedicated thread takes first packet from cyclic queue", "Scanner");
					}
#endif
#endif
				} else if (status == CYCLIC_PACKET_QUEUE_ERROR_EMPTY_QUEUE ||
						   status == CYCLIC_PACKET_QUEUE_ERROR_SYNC) {
#ifdef SCANNER_STATISTICS
					num_failed_dequeue_since_alert++;
#endif
					hp.last_idx_in_root = 0;
					if (next_writer_id == last_writer_id) {

						if (scanner->finishing_alert_mode) {
							if (finish_round < NUM_OF_FINISH_ROUNDS) {
								finish_round++;
								continue;
							} else {
								finish_round = 0;
								next_writer_id = 0;
								scanner->finishing_alert_mode = 0;
							}
						}

						packet = (Packet*)list_dequeue(scanner->packet_queue, &status);
						if (status != LIST_STATUS_DEQUEUE_SUCCESS && scanner->done) {
#ifdef GLOBAL_TIMING
#ifdef EXTRA_EVENTS
							if (!dedicated_stopped) {
								dedicated_stopped = 1;
								global_timer_set_event(&(scanner->manager->gtimer), "Dedicated thread is stopping", "Scanner");
							}
#endif
#endif
							break;
						} else if (status == LIST_STATUS_DEQUEUE_EMPTY) {
							multicore_manager_stop_all(scanner->manager);
						}
					} else {
						continue;
					}
				} else {
					// Something got wrong in cyclic queue
					fprintf(stderr, "FATAL ERROR: Dequeue from lock-free queue failed (reader=%d, writer=%d) [Error: %d]!\n", scanner->scanner_id, last_writer_id, status);
					exit(1);
				}
			} while (status != LIST_STATUS_DEQUEUE_SUCCESS);
		}

		if (scanner->done)
			break;

		/*
		do {
			packets[last] = (Packet*)list_dequeue(scanner->packet_queue, &status);
			lastInsLocation = last;
			last = (last + 1) % PACKET_READ_BUFFER_SIZE;
			count++;
		} while (count < PACKET_READ_BUFFER_SIZE && status == LIST_STATUS_DEQUEUE_SUCCESS && packets[lastInsLocation]);

		if (status != LIST_STATUS_DEQUEUE_SUCCESS || !packets[lastInsLocation]) {
			count--;
		}

		if (count == 0) {
			break;
		}
		*/
		/*
		///////
		if (status == LIST_STATUS_DEQUEUE_SUCCESS && packet) {
			free(packet->data);
			free(packet);
			continue;
		} else {
			break;
		}
		///////
		*/

		if (status == LIST_STATUS_DEQUEUE_SUCCESS && packet) {
		//while (count > 0) {
		//	packet = packets[first];
		//	if (!packet) {
		//		fprintf(stderr, "Packet is NULL!\n");
		//		exit(1);
		//	}
		//	packets[first] = NULL;
		//	first = (first + 1) % PACKET_READ_BUFFER_SIZE;
		//	count--;

			// Add bytes to byte count
			scanner->bytes_scanned += packet->size;
			if (scanner->alert_mode_active && !(scanner->manager->stopped)) {
				scanner->bytes_scanned_since_alert += packet->size;

				if (hp.last_idx_in_root > 0) {
//					if (hp.last_idx_in_root < 0 || hp.last_idx_in_root > 10000) {
//						fprintf(stderr, "Warning: weird value for hp.last_index_in_root (2): %d\n", hp.last_idx_in_root);
//					}

					scanner->bytes_scanned -= hp.last_idx_in_root;
					scanner->bytes_scanned_since_alert -= hp.last_idx_in_root;
				}

			}
#ifdef GLOBAL_TIMING
			bytes_scanned_recently += packet->size;
#endif
			num_total++;

			if (scanner->verbose) {
				printf("Scanning packet (size=%u) from ", packet->size);
				printIP(packet->source);
				printf(" to ");
				printIP(packet->dest);
				printf(" from time %u:\n", packet->time);
				printf("------------------------------------\n");
			}
			if (scanner->finishing_alert_mode || !(scanner->isTableMachine)  || scanner->manager->num_of_regular_threads == 0) {
				if (scanner->manager->dedicated_use_compressed) {
					retval = match(scanner->machine, packet->data, packet->size, scanner->verbose, &(scanner->stats), last_idx_in_root, scanner->drop);
				} else {
#ifndef SCANNER_STATISTICS
					retval = matchTableMachine_no_trasfer(
							scanner->tableMachine, scanner->manager,
							packet->data,
							packet->size,
							scanner->verbose,
							scanner->drop);
#else
					is_heavy = 0;
					retval = matchTableMachine(
							scanner->tableMachine, scanner->manager, 0,
							packet->data,
							packet->size,
							scanner->verbose,
							NULL, NULL, NULL, NULL, &is_heavy, NULL, &uncommonRate);

					if (is_heavy) {
						num_heavy_since_alert++;
					} else {
						num_regular_since_alert++;
					}
					//if (packet->size > 20) {
						uncommonRateHistogram[(int)(uncommonRate * (UNCOMMON_HISTOGRAM_SIZE - 1))]++;
					//}
#endif
				}
#ifdef DROP_STATISTICS
				if (retval == -1) {
					// Packet was dropped
					dropped++;
				}
#endif
			} else {
				is_heavy = 0;
				last_idx_in_root = 0;
				match_table_result = matchTableMachine(
#ifdef COUNT_BY_DEPTH
#ifndef HYBRID_SCANNER
						(TableStateMachine*)(scanner->machine), NULL, FALSE,
#else
						scanner->tableMachine, scanner->manager, scanner->alert_mode_active,
#endif
						packet->data,
						packet->size,
						scanner->verbose,
						&numAccesses, accessesByDepth, accessesByState, NULL, &is_heavy, &last_idx_in_root, &uncommonRate);
#else
#ifdef PRINT_STATE_VISIT_HIST
#ifndef HYBRID_SCANNER
						(TableStateMachine*)(scanner->machine), NULL, FALSE,
#else
						scanner->tableMachine, scanner->manager, scanner->alert_mode_active,
#endif
						packet->data,
						packet->size,
						scanner->verbose,
						NULL, NULL, NULL, visits, &is_heavy, &last_idx_in_root, &uncommonRate);
#else
#ifndef HYBRID_SCANNER
						(TableStateMachine*)(scanner->machine), NULL, FALSE,
#else
						scanner->tableMachine, scanner->manager,
#ifdef DONT_TRANSFER_STOLEN
						(scanner->alert_mode_active && !dont_transfer_next),
#else
						scanner->alert_mode_active,
#endif
#endif
						packet->data,
						packet->size,
						scanner->verbose,
						NULL, NULL, NULL, NULL, &is_heavy, &last_idx_in_root, &uncommonRate);
#endif // PRINT_STATE_VISIT_HIST
#endif // COUNT_BY_DEPTH
#ifdef DONT_TRANSFER_STOLEN
				if (dont_transfer_next)
					dont_transfer_next--;

				if (dont_transfer_next == 0 && match_table_result == -1) {
#else
				if (match_table_result == -1) {
#endif
#ifdef GLOBAL_TIMING
					bytes_scanned_recently = bytes_scanned_recently - packet->size + last_idx_in_root;
#endif
#ifndef DROP_ALL_HEAVY
#ifndef COUNT_DROPPED
					scanner->bytes_scanned = scanner->bytes_scanned - packet->size + last_idx_in_root;
					scanner->bytes_scanned_since_alert = scanner->bytes_scanned_since_alert - packet->size + last_idx_in_root;
#endif
#endif

#ifdef DROP_ALL_HEAVY
#ifndef COUNT_DROPPED
					scanner->bytes_scanned = scanner->bytes_scanned - packet->size;
					scanner->bytes_scanned_since_alert = scanner->bytes_scanned_since_alert - packet->size;
#endif
#ifdef DROP_STATISTICS
					dropped++;
#endif
#else
					hp.last_idx_in_root = last_idx_in_root;
					hp.packet = packet;
					uncommonRate = -1;

					// Transfer the heavy packet
					dest_thread = multicore_manager_transfer_packet(scanner->manager, &hp, scanner->scanner_id);
#ifdef SCANNER_STATISTICS
					num_transferred_since_alert++;
#endif
					free_packet = 0;
					// Take back another packet
					for (i = 0; i < scanner->manager->packets_to_steal; i++) {
						next_packet = list_dequeue(scanner->manager->scanners[dest_thread]->packet_queue, &status);
						if (status == LIST_STATUS_DEQUEUE_SUCCESS && next_packet != NULL) {
							list_insert_first(scanner->packet_queue, next_packet);
#ifdef DONT_TRANSFER_STOLEN
							dont_transfer_next++;
#endif
#ifdef SCANNER_STATISTICS
							packets_taken_from_dedicated++;
#endif
#ifdef GLOBAL_TIMING
#ifdef EXTRA_EVENTS
							if (!first_packet_taken_from_dedicated) {
								first_packet_taken_from_dedicated = 1;
								global_timer_set_event(&(scanner->manager->gtimer), "First packet taken from foreign queue", "Scanner");
							}
#endif
#endif
						}
					}
#endif
				}
				if (is_heavy) {
					num_heavy++;
#ifdef ISOLATION_TEST
					total_heavy++;
					if (packet->source != 1 || packet->dest != 5) {
						// This packet is not marked as heavy
						false_pos++;
					}
#endif
				}
#ifdef ISOLATION_TEST
				else {
					total_light++;
					if (packet->source == 1 && packet->dest == 5) {
						// This packet is marked as heavy
						false_neg++;
					}
				}
#endif

#ifdef SCANNER_STATISTICS
				if (scanner->alert_mode_active && is_heavy && match_table_result != -1) {
					num_heavy_since_alert++;
				} else if (scanner->alert_mode_active && match_table_result != -1){
					num_regular_since_alert++;
				}
				//sumOfHpRates += uncommonRate;
				//packetsCounted++;
				if (/*scanner->alert_mode_active && */uncommonRate >= 0 && uncommonRate <= 1) {// && packet->size > 20) {
					//if (is_heavy) {
						//printf("foo\n");
					//}
					uncommonRateHistogram[(int)(uncommonRate * (UNCOMMON_HISTOGRAM_SIZE - 1))]++;
				}
#endif
			}

			// Update multicore manager
			if (num_total % SCANNER_NUM_PACKETS_TO_UPDATE_MANAGER == 0) {
				multicore_manager_update(scanner->manager, scanner->scanner_id, num_heavy, num_total);
				num_heavy = num_total = 0;
			}
#ifdef GLOBAL_TIMING
			if (num_total % GLOBAL_TIMER_SCANNER_REPORT_INTERVAL == 0) {
				global_timer_set(&(scanner->manager->gtimer), scanner->scanner_id, bytes_scanned_recently);
				bytes_scanned_recently = 0;
			}
#endif

			if (free_packet) {
				//free(packet->data);
				//free(packet);
				list_enqueue(&(scanner->free_queue), (void*)packet);
			}
		//}

		} else {
			//printf("*** here ***\n");
#ifdef GLOBAL_TIMING
#ifdef EXTRA_EVENTS
			if (!my_queue_is_empty) {
				my_queue_is_empty = 1;
				global_timer_set_event(&(scanner->manager->gtimer), "Queue is empty", "Scanner");
			}
#endif
#endif
			break;
		}

	}
#ifdef PAPI
	if (papi_ok && PAPI_stop(event_set, papi_values) != PAPI_OK) {
		fprintf(stderr, "Cannot stop PAPI for thread %d\n", scanner->scanner_id);
		papi_ok = 0;
	}
	free(papi_values);

	if (papi_ok) {
		printf("PAPI\t[%d]\tL1\t%lld\tL2\t%lld\tL3\t%lld\n", scanner->scanner_id, papi_values[0], papi_values[1], papi_values[2]);
	}
#endif
	endTiming(&(scanner->timer));

	//printf("[%d] Packets read since alert mode: %d\nPackets taken from dedicated: %d\n", scanner->scanner_id, packets_read_since_alert, packets_taken_from_dedicated);
#ifdef SCANNER_STATISTICS
	printf("[%d] Heavy since alert: %d. Regular since alert: %d. Transferred since alert: %d. Received since alert: %d (failed: %d). Stolen since alert mode: %d.\n", scanner->scanner_id, num_heavy_since_alert, num_regular_since_alert, num_transferred_since_alert, num_received_since_alert, num_failed_dequeue_since_alert, packets_taken_from_dedicated);

	//, (sumOfHpRates / packetsCounted));
/*
	sprintf(uncommonHistPrintBuff, "[%d] Uncommon histogram ", scanner->scanner_id);
	len = strlen(uncommonHistPrintBuff);
	for (i = 0; i < UNCOMMON_HISTOGRAM_SIZE; i++) {
		sprintf(&(uncommonHistPrintBuff[len]), "\t%5d", uncommonRateHistogram[i]);
		len += 6;
	}
	printf("%s\n", uncommonHistPrintBuff);
*/
	printf("[%d] Uncommon histogram", scanner->scanner_id);
	for (i = 0; i < UNCOMMON_HISTOGRAM_SIZE; i++) {
		printf("\t%5d", uncommonRateHistogram[i]);
	}
	printf("\n");

#endif
#ifdef DROP_STATISTICS
	printf("DROP\t%d\n", dropped);
#endif

	multicore_manager_scanner_done(scanner->manager);

#ifdef COUNT_BY_DEPTH

#define DEPTH_THRESHOLD_FOR_DAVID_RATIO 1

	long deep_accesses = 0;
	long total_accesses = 0;

	// Output results
/*
	// Make accessesByDepth cumulative
	for (idx = 1; idx < 200; idx++) {
		accessesByDepth[idx] += accessesByDepth[idx - 1];
	}

	printf("=========\n");
	for (idx = 0; idx < 200; idx++) {
		printf("Accesses to depth 0-%ld:\t%ld\n", idx, accessesByDepth[idx]);
	}
	printf("=========\n");

	// Find number of states in each depth
	memset(accessesByDepth, 0, sizeof(long) * 200);
	for (idx = 0; idx < ((TableStateMachine*)(scanner->machine))->numStates; idx++) {
		accessesByDepth[((TableStateMachine*)(scanner->machine))->depthMap[idx]]++;
	}
	for (idx = 0; idx < 200; idx++) {
		printf("States in depth %ld:\t%ld\n", idx, accessesByDepth[idx]);
	}
	printf("=========\n");
*/
	for (idx = 0; idx < 200; idx++) {
		printf("%ld\t%ld\n", idx, accessesByDepth[idx]);
		if (idx > DEPTH_THRESHOLD_FOR_DAVID_RATIO) {
			deep_accesses += accessesByDepth[idx];
		}
		total_accesses += accessesByDepth[idx];
	}

	//printf("Deep accesses: %ld\n", deep_accesses);
	//printf("Total accesses: %ld\n", total_accesses);
	//printf("David's ratio: %f\n", ((double)deep_accesses) / total_accesses);

	free(accessesByState);
#endif

#ifdef COUNT_CALLS
	printCounters();
#endif

#ifdef X_RATIO
	printXRatio();
#endif

#ifdef PRINT_STATE_VISIT_HIST
	for (idx = 0; idx < scanner->tableMachine->numStates; idx++) {
		printf("%d\t%d\t%d\n", idx, visits[idx], (scanner->tableMachine->depthMap[idx]));
	}
#endif

#ifdef ISOLATION_TEST
	printf("[%d] Total heavy: %d, Total light: %d, False pos.: %d, False neg.: %d, Heavy rate: %1.3f\n", scanner->scanner_id, total_heavy, total_light, false_pos, false_neg, (((double)total_heavy) / (total_heavy + total_light)));
#endif

	return NULL;	
}

#ifdef HYBRID_SCANNER
void scanner_init(ScannerData *scanner, int id, MulticoreManager *manager, StateMachine *machine, TableStateMachine *tableMachine, int isTableMachine, LinkedList *packet_queue, int verbose, int drop) {
#else
void scanner_init(ScannerData *scanner, int id, MulticoreManager *manager, StateMachine *machine, int isTableMachine, LinkedList *packet_queue, int verbose) {
#endif
	scanner->scanner_id = id;
	scanner->done = 0;
	scanner->machine = machine;
#ifdef HYBRID_SCANNER
	scanner->tableMachine = tableMachine;
	//printf("Table machine address upon scanner init: %p\n", tableMachine);
#endif
	scanner->isTableMachine = isTableMachine;
	scanner->packet_queue = packet_queue;
	list_init(&(scanner->free_queue));
	scanner->verbose = verbose;
	scanner->stats.totalFailures = 0;
	scanner->stats.totalGotos = 0;
	scanner->manager = manager;
	scanner->alert_mode_active = 0;
	scanner->finishing_alert_mode = 0;
	scanner->drop = drop;
}

int scanner_start(ScannerData *scanner) {
	return pthread_create(&(scanner->thread_id), NULL, start_scanner_thread, (void*)scanner);
}

int scanner_start_with_affinity(ScannerData *scanner, int cpuid) {
	int res;
	CPU_ZERO(&(scanner->cpuset));
	CPU_SET(cpuid, &(scanner->cpuset));

	pthread_attr_init(&(scanner->thread_attr));
	pthread_attr_setaffinity_np(&(scanner->thread_attr), sizeof(cpu_set_t), &(scanner->cpuset));
	pthread_attr_setscope(&(scanner->thread_attr), PTHREAD_SCOPE_SYSTEM);

	res = pthread_create(&(scanner->thread_id), &(scanner->thread_attr), start_scanner_thread, (void*)scanner);

	/*
	if (res) {
		res = pthread_setaffinity_np((scanner->thread_id), sizeof(cpu_set_t), &(scanner->cpuset));
		return res;
	} else {
		return res;
	}
	*/
	return res;
}

void scanner_join(ScannerData *scanner) {
	Packet *p;
	int status;
	pthread_join(scanner->thread_id, NULL);
	pthread_attr_destroy(&(scanner->thread_attr));

	list_set_done(&(scanner->free_queue));
	do {
		p = (Packet*)list_dequeue(&(scanner->free_queue), &status);
		if (status == LIST_STATUS_DEQUEUE_SUCCESS && p) {
			free(p->data);
			free(p);
		}
	} while (status == LIST_STATUS_DEQUEUE_SUCCESS);
	list_destroy(&(scanner->free_queue), 0);
}

void scanner_set_machine_type(ScannerData *scanner, enum MACHINE_TYPE type) {
	switch (type) {
	case MACHINE_TYPE_NAIVE:
		scanner->isTableMachine = TRUE;
		break;
	case MACHINE_TYPE_COMPRESSED:
		scanner->isTableMachine = FALSE;
		break;
	}
}

void scanner_set_alert_mode(ScannerData *scanner, int alert_mode_active) {
	if (scanner->alert_mode_active && !alert_mode_active) {
		scanner->finishing_alert_mode = 1;
	}
	scanner->alert_mode_active = alert_mode_active;
}
