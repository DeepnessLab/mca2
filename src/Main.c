/*
 * Test.c
 *
 *  Created on: Jan 11, 2011
 *      Author: yotamhc
 */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "Common/Types.h"
#include "StateMachine/StateMachine.h"
#include "StateMachine/StateMachineGenerator.h"
#include "DumpReader/DumpReader.h"
#include "StateMachine/StateMachineDumper.h"
#include "StateMachine/TableStateMachineGenerator.h"

#include "AhoCorasick/FindLP.h"

/*
#define LOG_PATH "/Users/yotamhc/Documents/Study/CompactDFA/Code-Yaron/DARPA_eval_b/sample_data01.tcpdump.bin"
#define LOG_PATH_TEST "/Users/yotamhc/Documents/Study/CompactDFA/Code-Yaron/DARPA_eval_b/onepacket.tcpdump.bin"
#define PATTERNS_PATH "/Users/yotamhc/Documents/Study/CompactDFA/Code-Yaron/SnortPatterns.txt"
#define TEST_PATH "patterns.txt"

#define DUMP_PATH "/Users/yotamhc/Documents/Study/CompactDFA/Code-Yaron/Dumps/snort"
*/
#define USAGE "Usage: %s [-c:file] [-d:fileprefix] [-r:fileprefix] [-s:file] [-a:file] [-l:value] [-b:value] [-m:value] [-v] [-t] [-f:file]\n \
\t -c : generate database from given input pattern file. Optional flags for this options:\n \
\t\t -l : max goto transitions threshold for linear encoded states (default: 4)\n \
\t\t -b : max goto transitions threshold for bitmap encoded states (default: 64)\n \
\t -d : dump database to files at given path (you should specify file prefix). This option must be used together with -c.\n \
\t -r : read database from dump file at given path (you should specify file prefix). Cannot be used with -c or -d.\n \
\t -s : scan the given packet log file. Must be used together with either -c or -r but should not be used with -d. Optional flags:\n \
\t\t -t : time the process and output timing result\n \
\t\t -m : use specified number of scanning threads (default: 1).\n \
\t\t -n : repeat trace as the specified number of times (default: 1).\n \
\t\t To create such file from a TCPDUMP output file, use the separate tool.\n \
\t -a : generate full table state machine from given input pattern file. This machine cannot be saved to disk.\n \
\t\t You must use it to scan a packet log file using the -s option.\n \
\t\t -f : specify input file for reordering of common/uncommon states (only valid with -a).\n \
\t\t -p : specify number of packets to steal by regular threads.\n	\
\t -u : dedicated threads use compressed automaton.\n \
\t -A : alert mode level thresholds (list of float values in [0,1], separated with commas, no whitespace) .\n \
\t -w : work group size.\n \
\t -W : max number of dedicated work groups.\n \
\t -C : number of common states.\n	\
\t -U : threshold (percents in [0,100]) of uncommon state rate in a packet to declare it as heavy.\n \
\t -D : drop packets in dedicated if not visiting root for given number of state visits.\n \
*** If compiled on HYBRID_SCANNER mode, both -a and -r must be supplied! ***\n"

// *** KEEP THE ORDER OF THE FOLLOWING DEFINITIONS! ***
// String options
#define OPTION_CREATE 		'c'
#define OPTION_DUMP			'd'
#define OPTION_READ			'r'
#define OPTION_SCAN			's'
#define OPTION_FULLTBL		'a'
#define OPTION_COMMON		'f'
#define OPTION_THRESHOLDS 	'A'

// Numeric options
#define OPTION_MAXLE		'l'
#define OPTION_MAXBM		'b'
#define OPTION_THREADS		'm'
#define OPTION_STEAL		'p'
#define OPTION_REPEAT		'n'
#define OPTION_WG_SIZE		'w'
#define OPTION_MAX_WGS		'W'
#define OPTION_COMMON_NUM	'C'
#define OPTION_UNCOMMON_LIM	'U'
#define OPTION_DROP_LEN		'D'

// Boolean options
#define OPTION_VERBOSE		'v'
#define OPTION_TIMING		't'
#define OPTION_USE_COMP		'u'

#define OPTION_SEPARATOR ':'

#define IDX_CREATE 		0
#define IDX_DUMP		1
#define IDX_READ		2
#define IDX_SCAN		3
#define IDX_FULLTBL		4
#define IDX_COMMON		5
#define IDX_THRESHOLDS	6
#define IDX_MAXLE		7
#define IDX_MAXBM		8
#define IDX_THREADS		9
#define IDX_STEAL		10
#define IDX_REPEAT		11
#define IDX_WG_SIZE		12
#define IDX_MAX_WGS		13
#define IDX_COMMON_NUM	14
#define IDX_UNCOMMON_LIM 15
#define IDX_DROP_LEN	16
#define IDX_VERBOSE		17
#define IDX_TIMING		18
#define IDX_USE_COMP 	19

#define NUM_OF_OPTIONS 			20
#define NUM_OF_STRING_OPTIONS	7
#define NUM_OF_NUMERIC_OPTIONS	10

#define DEFAULT_MAX_GOTOS_LE 4
#define DEFAULT_MAX_GOTOS_BM 64
#define DEFAULT_NUM_THREADS 1
#define DEFAULT_STEAL 1
#define DEFAULT_REPEAT 1
#define DEFAULT_WG_SIZE 2
#define DEFAULT_MAX_WGS 1
#define DEFAULT_COMMON_NUM 512
#define DEFAULT_UNCOMMON_LIM 30
#define DEFAULT_DROP_LEN 0

void showUsage(const char *filename) {
	printf(USAGE, filename);
	exit(1);
}

int checkOptionsValidity(int *options) {
	if (!options[IDX_CREATE] && !options[IDX_READ] && !options[IDX_FULLTBL])
		return 0;
	if ((options[IDX_CREATE] || options[IDX_DUMP]) && options[IDX_READ])
		return 0;
	if (options[IDX_DUMP] && !options[IDX_CREATE])
		return 0;
	if (options[IDX_SCAN] && !options[IDX_CREATE] && !options[IDX_READ] && !options[IDX_FULLTBL])
		return 0;
	if (options[IDX_DUMP] && options[IDX_SCAN])
		return 0;
	if (!options[IDX_CREATE] && (options[IDX_MAXLE] || options[IDX_MAXBM]))
		return 0;
	if (options[IDX_VERBOSE] && !options[IDX_SCAN])
		return 0;
	if (options[IDX_THREADS] && !options[IDX_SCAN])
		return 0;
#ifndef HYBRID_SCANNER
	if (options[IDX_FULLTBL] && (!options[IDX_SCAN] || options[IDX_CREATE] || options[IDX_DUMP] || options[IDX_READ]))
		return 0;
#else
	if (options[IDX_FULLTBL] && (!options[IDX_SCAN] || options[IDX_CREATE] || options[IDX_DUMP]))
		return 0;
#endif
	if (options[IDX_COMMON] && !options[IDX_FULLTBL])
		return 0;
	return 1;
}

double *parseThresholds(const char *input, double *out) {
	int i, j, last;
	char buff[128];

	last = 0;
	j = 0;

	strcpy(buff, input);

	for (i = 0; buff[i] != '\0'; i++) {
		if (buff[i] == ',') {
			buff[i] = '\0';
			out[j++] = atof(&(buff[last]));
			last = i + 1;
		}
	}
	return out;
}

int main(int argc, const char *argv[]) {
/*
	main_findLongestPath();
	return 0;

 */
	StateMachine *machine;
#ifdef HYBRID_SCANNER
	TableStateMachine *tableMachine;
	double thresholds[128];
#endif
	int options[NUM_OF_OPTIONS]; // [ c, d, r, s, a, f, l, b, m, p, n, v, t, u ]
	const char *paths[NUM_OF_STRING_OPTIONS];
	int values[NUM_OF_OPTIONS];
	int i, idx;

	machine = NULL;

	if (argc < 3 || argc >= NUM_OF_OPTIONS) {
		showUsage(argv[0]);
	}

	memset(options, 0, NUM_OF_OPTIONS * sizeof(int));
	memset(paths, 0, NUM_OF_STRING_OPTIONS * sizeof(char*));
	memset(values, 0, NUM_OF_OPTIONS * sizeof(int));

	idx = -1;

	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			showUsage(argv[0]);
		}

		switch (argv[i][1]) {
		case OPTION_CREATE:
			idx = IDX_CREATE;
			break;
		case OPTION_DUMP:
			idx = IDX_DUMP;
			break;
		case OPTION_READ:
			idx = IDX_READ;
			break;
		case OPTION_SCAN:
			idx = IDX_SCAN;
			break;
		case OPTION_FULLTBL:
			idx = IDX_FULLTBL;
			break;
		case OPTION_COMMON:
			idx = IDX_COMMON;
			break;
		case OPTION_THRESHOLDS:
			idx = IDX_THRESHOLDS;
			break;
		case OPTION_MAXLE:
			idx = IDX_MAXLE;
			break;
		case OPTION_MAXBM:
			idx = IDX_MAXBM;
			break;
		case OPTION_THREADS:
			idx = IDX_THREADS;
			break;
		case OPTION_STEAL:
			idx = IDX_STEAL;
			break;
		case OPTION_REPEAT:
			idx = IDX_REPEAT;
			break;
		case OPTION_WG_SIZE:
			idx = IDX_WG_SIZE;
			break;
		case OPTION_MAX_WGS:
			idx = IDX_MAX_WGS;
			break;
		case OPTION_COMMON_NUM:
			idx = IDX_COMMON_NUM;
			break;
		case OPTION_UNCOMMON_LIM:
			idx = IDX_UNCOMMON_LIM;
			break;
		case OPTION_DROP_LEN:
			idx = IDX_DROP_LEN;
			break;
		case OPTION_VERBOSE:
			idx = IDX_VERBOSE;
			break;
		case OPTION_TIMING:
			idx = IDX_TIMING;
			break;
		case OPTION_USE_COMP:
			idx = IDX_USE_COMP;
			break;
		default:
			showUsage(argv[0]);
		}

		options[idx] = 1;

		if (idx < (NUM_OF_NUMERIC_OPTIONS + NUM_OF_STRING_OPTIONS)) {
			// c, d, r, s, a, f, l, b, m, p
			if (argv[i][2] != OPTION_SEPARATOR) {
				showUsage(argv[0]);
			}

			if (idx < NUM_OF_STRING_OPTIONS) {
				// String options
				// c, d, r, s, a, f
				paths[idx] = &(argv[i][3]);
			} else {
				// Numeric options
				// l, b, m, p, n
				values[idx] = atoi(&(argv[i][3]));
			}
		} else {
			// Boolean options
			// v, t, u
			// do nothing here
		}
	}
	if (!checkOptionsValidity(options)) {
		showUsage(argv[0]);
	}

	if (!options[IDX_MAXLE]) {
		values[IDX_MAXLE] = DEFAULT_MAX_GOTOS_LE;
	}
	if (!options[IDX_MAXBM]) {
		values[IDX_MAXBM] = DEFAULT_MAX_GOTOS_BM;
	}
	if (!options[IDX_THREADS]) {
		values[IDX_THREADS] = DEFAULT_NUM_THREADS;
	}
	if (!options[IDX_STEAL]) {
		values[IDX_STEAL] = DEFAULT_STEAL;
	}
	if (!options[IDX_REPEAT]) {
		values[IDX_REPEAT] = DEFAULT_REPEAT;
	}
	if (!options[IDX_WG_SIZE]) {
		values[IDX_WG_SIZE] = DEFAULT_WG_SIZE;
	}
	if (!options[IDX_MAX_WGS]) {
		values[IDX_MAX_WGS] = DEFAULT_MAX_WGS;
	}
	if (!options[IDX_COMMON_NUM]) {
		fprintf(stderr, "Warning: no value is given as number of common states (-C), using default: %d.\n", DEFAULT_COMMON_NUM);
		values[IDX_COMMON_NUM] = DEFAULT_COMMON_NUM;
	}
	if (!options[IDX_UNCOMMON_LIM]) {
		fprintf(stderr, "Warning: no value is given as a threshold of uncommon states to mark heavy packets (-U), using default: %d%%.\n", DEFAULT_UNCOMMON_LIM);
		values[IDX_UNCOMMON_LIM] = DEFAULT_UNCOMMON_LIM;
	}
	if (!options[IDX_DROP_LEN]) {
		values[IDX_DROP_LEN] = DEFAULT_DROP_LEN;
	}

#ifdef HYBRID_SCANNER
	tableMachine = NULL;
#endif

	if (options[IDX_CREATE]) {
		machine = createStateMachine(paths[IDX_CREATE], values[IDX_MAXLE], values[IDX_MAXBM], options[IDX_VERBOSE]);
		//machine = createSimpleStateMachine(paths[0], values[0], values[1], options[8]);
#ifndef HYBRID_SCANNER
	} else if (options[IDX_READ]) {
		machine = createStateMachineFromDump(paths[IDX_READ]);
	} else if (options[IDX_FULLTBL]) {
		machine = (StateMachine*)generateTableStateMachine(paths[IDX_FULLTBL], values[IDX_COMMON_NUM], 0, NULL, paths[IDX_COMMON], options[IDX_VERBOSE]);
	}
#else
	} else if (options[IDX_READ] && options[IDX_FULLTBL]) {
		if (paths[IDX_COMMON] == NULL) {
			fprintf(stderr, "Warning: no file was specified for common states reordering!\n");
			values[IDX_COMMON_NUM] = __INT32_MAX__;
		}
		machine = createStateMachineFromDump(paths[IDX_READ]);
		tableMachine = generateTableStateMachine(paths[IDX_FULLTBL], values[IDX_COMMON_NUM], ((double)values[IDX_UNCOMMON_LIM])/100.0, NULL, paths[IDX_COMMON], options[IDX_VERBOSE]);
	}
#endif

	if (options[IDX_DUMP]) {
		dumpStateMachine(machine, paths[IDX_DUMP]);
	} else if (options[IDX_SCAN]) {
#ifdef HYBRID_SCANNER
		if (tableMachine == NULL) {
			fprintf(stderr, "ERROR: Must specify both -r and -a for hybrid scan mode\n");
			return 1;
		} else {
			inspectDumpFile(paths[IDX_SCAN], values[IDX_REPEAT], machine, tableMachine, 1, options[IDX_VERBOSE], options[IDX_TIMING], values[IDX_THREADS],
					values[IDX_STEAL], options[IDX_USE_COMP], values[IDX_WG_SIZE], values[IDX_MAX_WGS], parseThresholds(paths[IDX_THRESHOLDS], thresholds), values[IDX_DROP_LEN]);
		}
#else
		inspectDumpFile(paths[IDX_SCAN], values[IDX_REPEAT], machine, options[IDX_FULLTBL], options[IDX_VERBOSE], options[IDX_TIMING], values[IDX_THREADS]);
#endif
		//runTest(machine, options[4]);
	}
	//machine = createStateMachine(PATTERNS_PATH);
	//dumpStateMachine(machine, "/Users/yotamhc/Documents/Study/CompactDFA/Code-Yaron/Dumps/snort2");

	//match(machine, "this is a short but important test with many 33D9A761-90C8-11D0-BD43-00A0C911CE86 letters. you don't see<----------- Fin du Fichier ----------- > tests like this one every day. it is special.");
	//inspectDumpFile(LOG_PATH, machine);
#ifdef HYBRID_SCANNER
	if (machine != NULL)
		destroyStateMachine(machine);
	if (tableMachine != NULL)
		destroyTableStateMachine(tableMachine);
#else
	if (machine) {
		if (options[IDX_FULLTBL]) {
			destroyTableStateMachine((TableStateMachine*)machine);
		} else {
			destroyStateMachine(machine);
		}
	}
#endif

	if (options[IDX_VERBOSE]) {
		printf("Process completed\n");
	}

	return 0;
}
