/*
 * Implementation of a packetreader
 */

#include <stdlib.h>
#include <stdio.h>
#include "../DumpReader.h"
#include "PacketReader.h"

#define HEADER_SIZE 16

#define DUMP_PATH_DARPA "/home/yotam/Dumps/dump.bin"
#define DUMP_PATH_ALEXA "/home/yotam/Dumps/alexa.bin"
#define DUMP_PATH_MATCH_000 "/home/yotam/Dumps/dump_test_cf000.bin"
#define DUMP_PATH_MATCH_010 "/home/yotam/Dumps/dump_test_cf010.bin"
#define DUMP_PATH_MATCH_020 "/home/yotam/Dumps/dump_test_cf020.bin"
#define DUMP_PATH_MATCH_030 "/home/yotam/Dumps/dump_test_cf030.bin"
#define DUMP_PATH_MATCH_040 "/home/yotam/Dumps/dump_test_cf040.bin"
#define DUMP_PATH_MATCH_050 "/home/yotam/Dumps/dump_test_cf050.bin"
#define DUMP_PATH_MATCH_060 "/home/yotam/Dumps/dump_test_cf060.bin"
#define DUMP_PATH_MATCH_070 "/home/yotam/Dumps/dump_test_cf070.bin"
#define DUMP_PATH_MATCH_080 "/home/yotam/Dumps/dump_test_cf080.bin"
#define DUMP_PATH_MATCH_090 "/home/yotam/Dumps/dump_test_cf090.bin"
#define DUMP_PATH_MATCH_100 "/home/yotam/Dumps/dump_test_cf100.bin"

#define DUMP_NEW_HR020_H100_L000 "/home/yotam/Dumps/dump_new_mixed_hr020_h100_l000.bin"
#define DUMP_NEW_HR020_H100_L005 "/home/yotam/Dumps/dump_new_mixed_hr020_h100_l005.bin"
#define DUMP_NEW_HR020_H100_L010 "/home/yotam/Dumps/dump_new_mixed_hr020_h100_l010.bin"
#define DUMP_NEW_HR020_H020_L000 "/home/yotam/Dumps/dump_new_mixed_hr020_h020_l000.bin"
#define DUMP_NEW_HR020_H020_L005 "/home/yotam/Dumps/dump_new_mixed_hr020_h020_l005.bin"
#define DUMP_NEW_HR020_H020_L010 "/home/yotam/Dumps/dump_new_mixed_hr020_h020_l010.bin"

#define DUMP_NEW_HR032_H100_L000 "/home/yotam/Dumps/dump_new_mixed_hr032_h100_l000.bin"
#define DUMP_NEW_HR033_H100_L000 "/home/yotam/Dumps/dump_new_mixed_hr033_h100_l000.bin"
#define DUMP_NEW_HR034_H100_L000 "/home/yotam/Dumps/dump_new_mixed_hr034_h100_l000.bin"

#define DUMP_TEMP "/home/yotam/Dumps/alexa.bin"

unsigned int readUIntValue(uchar *buff, int idx) {
	unsigned int res;
	int i;
	res = 0;
	for (i = 0; i < 4; i++) {
		res <<= 8;
		res |= buff[idx + i] & 0x0FF;
	}
	return res;
}

#ifdef FORCE_TRACE
#define REPEAT_SINGLE FORCED_TRACE_REPEAT
#define REPEAT_SEQUENCE 1
#define NUM_OF_MIXED_DUMPS 1
#else
#define REPEAT_SINGLE 1
#define REPEAT_SEQUENCE 2
#define NUM_OF_MIXED_DUMPS 1
#endif

Packet *clone_packet(Packet *src) {
	Packet *p;
	char *data;

	p = (Packet*)malloc(sizeof(Packet));
	data = (char*)malloc(sizeof(char) * src->size);

	memcpy(data, src->data, sizeof(char) * src->size);

	p->data = data;
	p->dest = src->dest;
	p->size = src->size;
	p->source = src->source;
	p->time = src->time;

	return p;
}

void *start_packetreader_thread(void *param) {
	PacketReaderData *packetreader;
	Packet p;
	//Packet *packetPtr;
	FILE *f;
	uchar header[HEADER_SIZE];
	char *data;
	int len, i;
	int count, q;
	LinkedList *curr_queue;
/*
#ifdef MIXED_TRAFFIC
	char *dumps[NUM_OF_MIXED_DUMPS] = {
#ifndef FORCE_TRACE
			//DUMP_PATH_DARPA,
			//DUMP_PATH_ALEXA,
			//DUMP_NEW_HR020_H100_L000//,
			DUMP_NEW_HR020_H020_L000//,
			//DUMP_NEW_HR033_H100_L000//,
			//DUMP_PATH_DARPA
#else
			FORCED_TRACE
#endif
	};
	int j, k;
	int random_start;
#endif
*/
//	count = 0;
//	q = 0;

	srand48(time(NULL));

	packetreader = (PacketReaderData*)param;
/*
#ifdef MIXED_TRAFFIC
	for (j = 0; j < REPEAT_SEQUENCE; j++) {
		for (k = 0; k < NUM_OF_MIXED_DUMPS; k++) {
			random_start = -1;
			//if (k % 2 == 0) {
				// Only for DARPA
				//random_start = 20000;
			//}
#endif
*/
			for (i = 0; i < packetreader->repeat; i++) {
				count = 0;
				q = 0;
				f = fopen(
/*
#ifdef MIXED_TRAFFIC
						dumps[k],
#else
						packetreader->path,
#endif
*/
						packetreader->path,
						"rb");
				if (!f) {
					fprintf(stderr, "Cannot read file %s\n",
/*
#ifdef MIXED_TRAFFIC
							dumps[k]
#else
							packetreader->path
#endif
*/
							packetreader->path
					);
					exit(1);
				}

				while (1) {

					//if (count == 790924) {
					//	break;
					//}

					len = fread(header, 1, HEADER_SIZE, f);
					if (len < HEADER_SIZE) {
						// EOF?
						break;
					}

					//p = (Packet*)malloc(sizeof(Packet));

					p.size = readUIntValue(header, 0);
					p.source = readUIntValue(header, 4);
					p.dest = readUIntValue(header, 8);
					p.time = readUIntValue(header, 12);

					//printf("ID: %d\tSize: %u\n", count, p->size);

					if (p.size <= 0 || p.size > 1000000) {
						fprintf(stderr, "Invalid packet size: %u\n", p.size);
						continue;
					}

					data = (char*)malloc(sizeof(char) * (p.size + 1));
					if (fread(data, 1, p.size, f) != p.size) {
						fprintf(stderr, "ERROR: Failed to read packet from file\n");
						exit(1);
					}
					data[p.size] = '\0';

					p.data = data;
/*
					if (random_start > 0 && count < random_start) {
						count++;
						free(data);
						//free(p);
						continue;
					}
*/
					for (q = 0; q < packetreader->num_queues; q++) {
						curr_queue = packetreader->packet_queues[q];
//						q = (q + 1) % (packetreader->num_queues);

						list_enqueue(curr_queue, clone_packet(&p));
					}
					free(data);
					count++;

					packetreader->size += p.size * packetreader->num_queues;
					packetreader->sizeWithHeaders += (p.size + HEADER_SIZE) * packetreader->num_queues;
				}

				fclose(f);
			}
/*
#ifdef MIXED_TRAFFIC
		}
	}
#endif
*/
	for (q = 0; q < packetreader->num_queues; q++) {
		list_set_done(packetreader->packet_queues[q]);
	}

	//printf("Total packets: %d\n", count);

	return NULL;
}

void packetreader_init(PacketReaderData *packetreader, const char *path, int repeat, LinkedList *packet_queues, int num_queues) {
	int i;
	packetreader->done = 0;
	packetreader->path = path;
	packetreader->size = 0;
	packetreader->sizeWithHeaders = 0;
	packetreader->repeat = repeat;

	for (i = 0; i < num_queues; i++) {
		packetreader->packet_queues[i] = &(packet_queues[i]);
	}
	for (; i < MAX_QUEUES; i++) {
		packetreader->packet_queues[i] = NULL;
	}

	packetreader->num_queues = num_queues;
}

int packetreader_start(PacketReaderData *packetreader) {
	return pthread_create(&(packetreader->thread_id), NULL, start_packetreader_thread, (void*)packetreader);
}

void packetreader_join(PacketReaderData *packetreader) {
	pthread_join(packetreader->thread_id, NULL);
}
