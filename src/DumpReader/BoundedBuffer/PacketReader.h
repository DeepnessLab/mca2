/*
 * PacketReader
 * Functions to initialize, start, stop and destroy a packetreader
 */

#include <pthread.h>
#include "LinkedList.h"

#define MAX_QUEUES 8

// PacketReader thread data
typedef struct packetreader_data {
	const char *path;
	long size;
	long sizeWithHeaders;
	LinkedList *packet_queues[MAX_QUEUES];		// A queue of packets read (output)
	int num_queues;
	pthread_t thread_id;		// Thread ID of this packetreader
	int done;					// Termination flag for this packetreader
	int repeat;
} PacketReaderData;

void packetreader_init(PacketReaderData *packetreader, const char *path, int repeat, LinkedList *packet_queues, int num_queues);

int packetreader_start(PacketReaderData *packetreader);

void packetreader_join(PacketReaderData *packetreader);
