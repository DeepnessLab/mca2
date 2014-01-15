/*
 * DumpReader.h
 *
 *  Created on: Jan 17, 2011
 *      Author: yotamhc
 */

#ifndef DUMPREADER_H_
#define DUMPREADER_H_

#include "../StateMachine/StateMachine.h"
#include "../StateMachine/TableStateMachine.h"

typedef struct {
	unsigned int size;
	unsigned int source;
	unsigned int dest;
	unsigned int time;
	char *data;
} Packet;

#ifdef HYBRID_SCANNER
void inspectDumpFile(const char *path, int repeat, StateMachine *machine, TableStateMachine *tableMachine, int isTableMachine,
		int verbose, int timing, int threads, int packets_to_steal, int dedicated_use_compressed,
		int work_group_size, int max_wgs, double *thresholds, int drop);
#else
void inspectDumpFile(const char *path, int repeat, StateMachine *machine, int isTableMachine, int verbose, int timing, int threads);
#endif
void runTest(StateMachine *machine, int isTableMachine);

#endif /* DUMPREADER_H_ */
