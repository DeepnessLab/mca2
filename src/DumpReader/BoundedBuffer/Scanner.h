/*
 * Scanner
 * Functions to initialize, start, stop and destroy a scanner
 */
#ifndef _SCANNER_H
#define _SCANNER_H

#define _GNU_SOURCE
#include <sched.h>
#include <pthread.h>
#include "LinkedList.h"
#include "../../StateMachine/StateMachine.h"
#ifdef HYBRID_SCANNER
#include "../../StateMachine/TableStateMachine.h"
#endif
#include "../../Common/Timer.h"

struct multicore_manager;

typedef struct scanner_data {
	int scanner_id;
	StateMachine *machine;
#ifdef HYBRID_SCANNER
	TableStateMachine *tableMachine;
#endif
	int isTableMachine;
	int verbose;
	LinkedList *packet_queue;
	LinkedList free_queue;
	MachineStats stats;
	pthread_t thread_id;
	pthread_attr_t thread_attr;
	cpu_set_t cpuset;
	int done;
	Timer timer;
	unsigned long bytes_scanned;
	unsigned long bytes_scanned_since_alert;
	struct multicore_manager *manager;
	int alert_mode_active;
	int finishing_alert_mode;
	int drop;
} ScannerData;

enum MACHINE_TYPE {
	MACHINE_TYPE_NAIVE,
	MACHINE_TYPE_COMPRESSED
};

#ifdef HYBRID_SCANNER
void scanner_init(ScannerData *scanner, int id, struct multicore_manager *manager, StateMachine *machine, TableStateMachine *tableMachine, int isTableMachine, LinkedList *packet_queue, int verbose, int drop);
#else
void scanner_init(ScannerData *scanner, int id, struct multicore_manager *manager, StateMachine *machine, int isTableMachine, LinkedList *packet_queue, int verbose);
#endif

int scanner_start(ScannerData *scanner);

int scanner_start_with_affinity(ScannerData *scanner, int cpuid);

void scanner_join(ScannerData *scanner);

void scanner_set_machine_type(ScannerData *scanner, enum MACHINE_TYPE type);

void scanner_set_alert_mode(ScannerData *scanner, int alert_mode_active);

#define GET_SCANNER_TRANSFER_RATE(scannerPtr) \
	GET_TRANSFER_RATE((scannerPtr)->bytes_scanned, &((scannerPtr)->timer))

#endif // _SCANNER_H
