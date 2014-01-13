/*
 * CyclicLockFreeQueue.c
 *
 *  Created on: Dec 15, 2011
 *      Author: yotam
 */

#include <stdlib.h>
#include <stdio.h>
#include "CyclicLockFreeQueue.h"

#define MAX_SEQUENCE_NUMBER 16777216 // 2^24

int cyclic_lock_free_queue_init(CyclicLockFreeQueue *q, int size) {
#ifndef LOCK_CYCLIC_QUEUE
	q->left = (unsigned int*)malloc(sizeof(unsigned int) * size);
	q->right = (unsigned int*)malloc(sizeof(unsigned int) * size);
	q->packets = (HeavyPacket*)malloc(sizeof(HeavyPacket) * size);
	memset(q->left, 0, sizeof(unsigned int) * size);
	memset(q->right, 0, sizeof(unsigned int) * size);
	q->read_idx = 0;
	q->write_idx = 0;
	q->read_sq = 1;
	q->write_sq = 1;
	q->size = size;
#else
	list_init(&(q->list));
	list_set_lock(&(q->list), 1);
	q->in = q->out = 0;
#endif
	return 0;
}

void cyclic_lock_free_queue_destroy(CyclicLockFreeQueue *q) {
#ifndef LOCK_CYCLIC_QUEUE
	free(q->left);
	free(q->right);
	free(q->packets);
#else
	list_destroy(&(q->list), 1);
#endif
}

/*
 * Dequeues an element from queue
 * Returns some error code or CYCLIC_PACKET_QUEUE_SUCEESS
 */
int cyclic_lock_free_queue_dequeue(CyclicLockFreeQueue *q, HeavyPacket *result) {
#ifndef LOCK_CYCLIC_QUEUE
	unsigned int left, right;

	// READER READS FROM RIGHT TO LEFT
	right = q->right[q->read_idx];
	*result = q->packets[q->read_idx];
	left = q->left[q->read_idx];
/*
	if (result == NULL) {
		printf("Found null value in queue (right=%u, left=%u, read_idx=%u, read_sq=%u)\n",
				right, left, q->read_idx, q->read_sq);
	}
*/
	//printf("|%u\t%u\t|%u\t|%u\t|\n", right, left, q->read_idx, q->read_sq);

	if (right == q->read_sq && right == left) {
		// OK
		q->read_idx = (q->read_idx + 1) % q->size;
		if (q->read_idx == 0) {
			q->read_sq = (q->read_sq + 1) % MAX_SEQUENCE_NUMBER;
		}
		return CYCLIC_PACKET_QUEUE_SUCEESS;
	} else if ((q->read_sq > 0  && right == (q->read_sq - 1)) ||
			   (q->read_sq == 0 && right == MAX_SEQUENCE_NUMBER - 1)) {
		// Empty queue
		return CYCLIC_PACKET_QUEUE_ERROR_EMPTY_QUEUE;
	} else if (right != left) {
		// Writer may be writing to this location right now
		return CYCLIC_PACKET_QUEUE_ERROR_SYNC;
	} else {
		// Other error, possible overflow
		printf("OVERFLOW: Read idx: %u, Read sq: %u, right: %u, left: %u\n", q->read_idx, q->read_sq, right, left);
		return CYCLIC_PACKET_QUEUE_ERROR_OVERFLOW;
	}
#else
	int status;
	HeavyPacket *p;
	p = (HeavyPacket*)list_dequeue(&(q->list), &status);
	if (status == LIST_STATUS_DEQUEUE_SUCCESS) {
		*result = *p;
		free(p);
		q->out++;
		return CYCLIC_PACKET_QUEUE_SUCEESS;
	} else {
		return CYCLIC_PACKET_QUEUE_ERROR_EMPTY_QUEUE;
	}
#endif
}

/*
 * Checks if queue is empty
 * Returns some error code or CYCLIC_PACKET_QUEUE_SUCEESS
 */
int cyclic_lock_free_queue_isempty(CyclicLockFreeQueue *q) {
#ifndef LOCK_CYCLIC_QUEUE
	unsigned int left, right;

	// READER READS FROM RIGHT TO LEFT
	right = q->right[q->read_idx];
	//*result = q->packets[q->read_idx];
	left = q->left[q->read_idx];
/*
	if (result == NULL) {
		printf("Found null value in queue (right=%u, left=%u, read_idx=%u, read_sq=%u)\n",
				right, left, q->read_idx, q->read_sq);
	}
*/
	//printf("|%u\t%u\t|%u\t|%u\t|\n", right, left, q->read_idx, q->read_sq);

	if (right == q->read_sq && right == left) {
		// OK
		return CYCLIC_PACKET_QUEUE_SUCEESS;
	} else if ((q->read_sq > 0  && right == (q->read_sq - 1)) ||
			   (q->read_sq == 0 && right == MAX_SEQUENCE_NUMBER - 1)) {
		// Empty queue
		return CYCLIC_PACKET_QUEUE_ERROR_EMPTY_QUEUE;
	} else if (right != left) {
		// Writer may be writing to this location right now
		return CYCLIC_PACKET_QUEUE_ERROR_SYNC;
	} else {
		// Other error, possible overflow
		printf("OVERFLOW: Read idx: %u, Read sq: %u, right: %u, left: %u\n", q->read_idx, q->read_sq, right, left);
		return CYCLIC_PACKET_QUEUE_ERROR_OVERFLOW;
	}
#else
	int status;
	status = list_is_empty(&(q->list));
	if (status) {
		return CYCLIC_PACKET_QUEUE_SUCEESS;
	} else {
		return CYCLIC_PACKET_QUEUE_ERROR_EMPTY_QUEUE;
	}
#endif
}

int cyclic_lock_free_queue_enqueue(CyclicLockFreeQueue *q, HeavyPacket *packet) {
#ifndef LOCK_CYCLIC_QUEUE
	q->left[q->write_idx] = q->write_sq;
	q->packets[q->write_idx] = *packet;
	q->right[q->write_idx] = q->write_sq;

	q->write_idx = (q->write_idx + 1) % q->size;

	if (q->write_idx == 0) {
		q->write_sq = (q->write_sq + 1) % MAX_SEQUENCE_NUMBER;
	}

	return CYCLIC_PACKET_QUEUE_SUCEESS;
#else
	HeavyPacket *p;
	p = (HeavyPacket*)malloc(sizeof(HeavyPacket));
	*p = *packet;
	list_enqueue(&(q->list), p);
	q->in++;
	return CYCLIC_PACKET_QUEUE_SUCEESS;
#endif
}

int cyclic_lock_free_queue_get_size(CyclicLockFreeQueue *q) {
#ifndef LOCK_CYCLIC_QUEUE
	return -1;
#else
	LinkedListItem *item;
	int count;

	count = 0;
	item = q->list.first;
	while (item) {
		count++;
		item = item->next;
	}
	return count;
#endif
}

/////////// TEST CODE
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define TEST_SIZE 10
#define TEST_NUM_OF_PACKETS 10
#define PRODUCER_WAIT_NANOSECONDS 10000000 // 10ms

void print_queue(CyclicLockFreeQueue *q) {
#ifndef LOCK_CYCLIC_QUEUE
	int i;
	for (i = 0; i < q->size; i++) {
		printf("%u\t[%u, %s]\t%u\n", q->left[i], q->packets[i].last_idx_in_root, q->packets[i].packet->data, q->right[i]);
	}
#else
	printf("Not supported\n");
#endif
}

typedef struct {
	CyclicLockFreeQueue *q;
	int stopped;
} ThreadParam;

void *start_producer_thread(void *param) {
	ThreadParam *t;
	int i;
	HeavyPacket hp;
	Packet *p;
	struct timespec ts;

	t = (ThreadParam*)param;
	ts.tv_sec = 0;
	ts.tv_nsec = PRODUCER_WAIT_NANOSECONDS;

	for (i = 0; i < TEST_NUM_OF_PACKETS; i++) {
		hp.last_idx_in_root = i;
		p = (Packet*)malloc(sizeof(Packet));
		p->data = (char*)malloc(sizeof(char) * 100);
		sprintf(p->data, "Packet %4d", i);
		p->size = 12;
		hp.packet = p;

		cyclic_lock_free_queue_enqueue(t->q, &hp);

		nanosleep(&ts, NULL);
	}

	t->stopped = 1;

	printf("Producer is done\n");

	pthread_exit(NULL);
}

void *start_consumer_thread(void *param) {
	ThreadParam *t;
	int status;
	HeavyPacket hp;

	t = (ThreadParam*)param;

	printf("|right\tleft\t|r_idx\t|r_sq\t|\n");

	while (1) {
		status = cyclic_lock_free_queue_dequeue(t->q, &hp);
		if (status == CYCLIC_PACKET_QUEUE_SUCEESS) {
			printf("Received packet: Last idx: %u, Payload: %s\n", hp.last_idx_in_root, hp.packet->data);
			free(hp.packet->data);
			free(hp.packet);
		} else if (status == CYCLIC_PACKET_QUEUE_ERROR_EMPTY_QUEUE) {
			if (t->stopped) {
				break;
			}
			printf("Queue is empty\n");
			sleep(1);
		} else {
			printf("Error in consumer: %d\n", status);
			break;
		}
	}

	printf("Consumer is done\n");

	pthread_exit(NULL);
}

void cyclic_queue_test() {
	CyclicLockFreeQueue q;
	pthread_t t1, t2;
	ThreadParam param;

	cyclic_lock_free_queue_init(&q, TEST_SIZE);

	param.q = &q;
	param.stopped = 0;

	pthread_create(&t1, NULL, start_producer_thread, (void*)(&param));
	pthread_create(&t2, NULL, start_consumer_thread, (void*)(&param));

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	cyclic_lock_free_queue_destroy(&q);
}

int _main() {
	cyclic_queue_test();
	return 0;
}
