/*
 * CyclicLockFreeQueue.h
 *
 *  Created on: Dec 15, 2011
 *      Author: yotam
 */

#ifndef CYCLICLOCKFREEQUEUE_H_
#define CYCLICLOCKFREEQUEUE_H_

#include "../DumpReader/DumpReader.h"
#include "HeavyPacket.h"
#ifdef LOCK_CYCLIC_QUEUE
#include "../DumpReader/BoundedBuffer/LinkedList.h"
#endif

#define CYCLIC_PACKET_QUEUE_SUCEESS 			0
#define CYCLIC_PACKET_QUEUE_ERROR_EMPTY_QUEUE 	1
#define CYCLIC_PACKET_QUEUE_ERROR_OVERFLOW 		2
#define CYCLIC_PACKET_QUEUE_ERROR_SYNC			3

typedef struct {
#ifndef LOCK_CYCLIC_QUEUE
	unsigned int *left;
	HeavyPacket *packets;
	unsigned int *right;
	unsigned int read_idx;
	unsigned int write_idx;
	unsigned int read_sq;
	unsigned int write_sq;
	unsigned int size;
#else
	LinkedList list;
	int in, out;
#endif
} CyclicLockFreeQueue;

int cyclic_lock_free_queue_init(CyclicLockFreeQueue *q, int size);
void cyclic_lock_free_queue_destroy(CyclicLockFreeQueue *q);
int cyclic_lock_free_queue_dequeue(CyclicLockFreeQueue *q, HeavyPacket *result);
int cyclic_lock_free_queue_enqueue(CyclicLockFreeQueue *q, HeavyPacket *value);
int cyclic_lock_free_queue_get_size(CyclicLockFreeQueue *q);
int cyclic_lock_free_queue_isempty(CyclicLockFreeQueue *q);

#endif /* CYCLICLOCKFREEQUEUE_H_ */
