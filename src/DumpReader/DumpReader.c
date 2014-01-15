/*
 * DumpReader.c
 *
 *  Created on: Jan 17, 2011
 *      Author: yotamhc
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
//#include <sys/time.h>
//#include <time.h>
//#include <unistd.h>
#include "DumpReader.h"
//#include "BoundedBuffer/Scanner.h"
#include "BoundedBuffer/PacketReader.h"
#include "../StateMachine/TableStateMachine.h"
#include "../Common/Timer.h"
#include "../Multicore/MulticoreManager.h"
#ifdef PAPI
#include <papi.h>
#endif

// #define HEADER_SIZE 16

/*
const unsigned int MASKS[] = { 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF };

unsigned int readIntValue(uchar *buff, int idx) {
	unsigned int res;
	int i;
	res = 0;
	for (i = 0; i < 4; i++) {
		res <<= 8;
		res |= buff[idx + i] & 0x0FF;
	}
	return res;
}

void printIP(unsigned int ip) {
	printf("%u.%u.%u.%u", ((ip & MASKS[0]) >> 24), ((ip & MASKS[1]) >> 16), ((ip & MASKS[2]) >> 8), ip & MASKS[3]);
}

void handlePacket(StateMachine *machine, Packet *p, int verbose) {
	if (verbose) {
		printf("Scanning packet (size=%u) from ", p->size);
		printIP(p->source);
		printf(" to ");
		printIP(p->dest);
		printf(" from time %u:\n", p->time);

		printf("------------------------------------\n");
	}
	match(machine, p->data, p->size, verbose);
}
*/

#ifdef HYBRID_SCANNER
void inspectDumpFile(const char *path, int repeat, StateMachine *machine, TableStateMachine *tableMachine, int isTableMachine,
		int verbose, int timing, int threads, int packets_to_steal, int dedicated_use_compressed,
		int work_group_size, int max_wgs, double *thresholds, int drop) {
#else
void inspectDumpFile(const char *path, int repeat, StateMachine *machine, int isTableMachine, int verbose, int timing, int threads) {
#endif
	double /*rate,*/ combinedRate, threadRate;//, rateWithHeaders;
	Timer t;
	long size;//, sizeWithHeaders;
	int i, cpuid;
#ifdef GLOBAL_TIMING
	GlobalTimerResult gtimer_result;
	int j;
#ifdef PRINT_GLOBAL_TIMER_EVENTS
	GlobalTimerEvent **events;
#endif
#endif
	ScannerData *scanners;
	PacketReaderData reader;
	LinkedList *packet_queues;

	MulticoreManager manager;

#ifdef COUNT_FAIL_PERCENT
	long totalFailures, totalGotos;
#endif

#ifdef PAPI
	if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
		fprintf(stderr, "Cannot init PAPI\n");
		exit(1);
	}

	if (PAPI_thread_init((unsigned long (*)(void))pthread_self) != PAPI_OK) {
		fprintf(stderr, "Cannot init PAPI for threads\n");
		exit(1);
	}
#endif

	packet_queues = (LinkedList*)malloc(sizeof(LinkedList) * threads);
	scanners = (ScannerData*)malloc(sizeof(ScannerData) * threads);

	for (i = 0; i < threads; i++) {
		list_init(&packet_queues[i]);
	}

	packetreader_init(&reader, path, repeat, packet_queues, threads);
	for (i = 0; i < threads; i++) {
#ifdef HYBRID_SCANNER
		scanner_init(&(scanners[i]), i, &manager, machine, tableMachine, isTableMachine, &packet_queues[i], verbose, drop);
#else
		scanner_init(&(scanners[i]), i, &manager, machine, isTableMachine, &packet_queues[i], verbose);
#endif
	}

#ifdef HYBRID_SCANNER
	multicore_manager_init(&manager, scanners, threads, work_group_size, max_wgs, packets_to_steal, dedicated_use_compressed);
	multicore_manager_set_thresholds(&manager, thresholds);
#else
	multicore_manager_init(&manager, scanners, threads, 1, threads, 0, 0);
#endif
	packetreader_start(&reader);

	packetreader_join(&reader);

#ifdef HYBRID_SCANNER
	multicore_manager_start(&manager);
#endif
#ifdef GLOBAL_TIMING
#ifdef PRINT_GLOBAL_TIMER_EVENTS
	events = NULL;
#endif
	global_timer_start(&(manager.gtimer));
#endif
	if (timing) {
		startTiming(&t);
	}

	for (i = 0; i < threads; i++) {
		// If CPUs are ordered [core0,core0,...,core0,core1,core1,...,core1,...]
		//cpuid = i;
		// If CPUs are ordered [core0,core1,...,coreN,core0,core1,...,coreN,...]
		cpuid = (i % 2 == 0) ? i / 2 : (threads + i) / 2;
		scanner_start_with_affinity(&(scanners[i]), cpuid);

		// If you use the next line, comment out the pthread_attr_destroy call in scanner_join!!!
		//scanner_start(&(scanners[i]));
	}

	for (i = 0; i < threads; i++) {
		scanner_join(&(scanners[i]));
	}

//	scanner_start(&(scanners[0]));
//	scanner_start(&(scanners[1]));
//	scanner_join(&(scanners[0]));
//	scanner_join(&(scanners[1]));

	if (timing) {
		endTiming(&t);
	}

#ifdef GLOBAL_TIMING
	global_timer_end(&(manager.gtimer));
#endif

#ifdef HYBRID_SCANNER
	multicore_manager_stop(&manager);

	multicore_manager_join(&manager);
#endif

#ifdef GLOBAL_TIMING
	global_timer_join(&(manager.gtimer));
	global_timer_get_results(&(manager.gtimer), &gtimer_result);
#endif

	if (timing) {
		//endTiming(&t);
		size = reader.size;
		//sizeWithHeaders = reader.sizeWithHeaders;
		//rate = GET_TRANSFER_RATE(size, &t);
		//rateWithHeaders = GET_TRANSFER_RATE(sizeWithHeaders, &t);

//		printf("Time(micros)\tData(No H)\tData(w/ H)\tRate(No H) Mb/s\tRate (w/ H) Mb/s\n");
		//printf("%8ld\t%9ld\t%9ld\t%5.4f\t%5.4f\n", t.micros, size, sizeWithHeaders, rate, rateWithHeaders);

		printf("TOTAL_BYTES\tTotal data scanned: %ld bytes\n", size);
		//printf("TOTAL_TIME\tTotal time: %ld ms\n", t.micros);
		//printf("TOTAL_THRPT\tTotal throughput: %5.4f Mbps\n", rate);

		combinedRate = 0;
		printf("Alert mode timer: %ld us\n", manager.alert_mode_timer.micros);
		for (i = 0; i < threads; i++) {
			if (0 && manager.alert_mode_used) {
				threadRate = GET_TRANSFER_RATE(scanners[i].bytes_scanned_since_alert, &(manager.alert_mode_timer));
			} else {
				threadRate = GET_SCANNER_TRANSFER_RATE(&(scanners[i]));
			}
			combinedRate += threadRate;
			printf("T_%2d_THRPT\t%5.4f\tMbps\t%lu\tB\t%lu\tB\t%ld\tus\n", i, threadRate, scanners[i].bytes_scanned, scanners[i].bytes_scanned_since_alert, scanners[i].timer.micros);
		}
		printf("COMB_THRPT\t%5.4f\tMbps\n", combinedRate);

#ifdef GLOBAL_TIMING
		//printf("\nGlobal timing:\n");
/*
		printf("Time\t");
		for (j = 0; j < manager.gtimer.intervals; j++) {
			printf("%6ld\t", gtimer_result.times[j]);
		}
		printf("\n");
*/
		for (i = 0; i < manager.gtimer.num_scanners; i++) {
			printf("T_%2d_GTIME\t", i);
			for (j = 0; j < manager.gtimer.intervals; j++) {
				printf("%5.3f\t", gtimer_result.results[gtimer_result.intervals * i + j]);
			}
			printf("\n");
		}

#ifdef PRINT_GLOBAL_TIMER_EVENTS
		j = global_timer_get_events(&(manager.gtimer), &events);
		if (j > 0) {
			printf("\nEvents:\n");
			for (i = 0; i < j; i++) {
				printf("Event %d: %s [Time: %d, Source: %s]\n", i, events[i]->text, events[i]->interval, events[i]->source);
			}
		}
#endif
#endif
	}
#ifdef COUNT_FAIL_PERCENT
	totalFailures = totalGotos = 0;
        for (i = 0; i < threads; i++) {
                totalFailures += scanners[i].stats.totalFailures;
		totalGotos += scanners[i].stats.totalGotos;
        }

        printf("Fail percent: %f\n", ((double)totalFailures) / (totalFailures + totalGotos));
        printf("Total failures: %ld, Total gotos: %ld\n", totalFailures, totalGotos);
#endif

    multicore_manager_destroy(&manager);
#ifdef GLOBAL_TIMING
    global_timer_destroy(&(manager.gtimer));
    global_timer_destroy_result(&gtimer_result);
#endif

    free(scanners);
    for (i = 0; i < threads; i++) {
    	//printf("Status of input-queue of thread %d: in=%d, out=%d\n", i, packet_queues[i].in, packet_queues[i].out);
    	list_destroy(&(packet_queues[i]), 1);
    }
    free(packet_queues);
}

#define TEST_INPUT "/Users/yotamhc/Documents/workspace/AC-NFA-C/victor/data.bin"

void runTest(StateMachine *machine, int isTableMachine) {
	FILE *file;
	int buffSize, len;
	char *buff;
	double rate;
	Timer t;
	MachineStats stats;
	int is_heavy, last_idx_in_root;
	double uncommonRate;

	stats.totalFailures = 0;
	stats.totalGotos = 0;

	file = fopen(TEST_INPUT, "rb");
	if (!file) {
		fprintf(stderr, "Error opening file for reading\n");
		exit(1);
	}

	fseek(file, 0L, SEEK_END);
	buffSize = ftell(file);
	fseek(file, 0L, SEEK_SET);

	buff = (char*)malloc(sizeof(char) * buffSize);
	if (buff == NULL) {
		fprintf(stderr, "Error allocating memory for buffer\n");
		exit(1);
	}
	len = fread(buff, sizeof(char), buffSize, file);
	if (len != buffSize) {
		fprintf(stderr, "Error reading data from file\n");
		exit(1);
	}

	t.micros = 0;
	if (isTableMachine) {
		startTiming(&t);
		matchTableMachine((TableStateMachine*)machine, NULL, FALSE, buff, buffSize, 1, NULL, NULL, NULL, NULL, &is_heavy, &last_idx_in_root, &uncommonRate);
		endTiming(&t);
	} else {
		startTiming(&t);
		match(machine, buff, buffSize, 0, &stats, 0, 0);
		endTiming(&t);
	}
	rate = GET_TRANSFER_RATE(buffSize, &t);

	printf("Time(micros)\tData(No H)\tData(w/ H)\tRate(No H) Mb/s\tRate (w/ H) Mb/s\n");
	printf("%8ld\t%9d\t%9d\t%5.4f\t%5.4f\n", t.micros, buffSize, buffSize, rate, rate);

	free(buff);

	fclose(file);
}


