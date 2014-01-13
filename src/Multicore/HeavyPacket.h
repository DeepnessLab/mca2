/*
 * HeavyPacket.h
 *
 *  Created on: Dec 20, 2011
 *      Author: yotam
 */

#ifndef HEAVYPACKET_H_
#define HEAVYPACKET_H_

#include "../DumpReader/DumpReader.h"

typedef struct {
	Packet *packet;
	unsigned int last_idx_in_root;
} HeavyPacket;

#define GET_HEAVY_PACKET_QUEUE(manager, regular_thread_idx, dedicated_thread_idx) \
	(((manager)->heavy_packet_queues)[(dedicated_thread_idx) * (manager)->num_scanners + (regular_thread_idx)])

HeavyPacket *heavy_packet_create(Packet *p, unsigned int last_idx_in_root);

void heavy_packet_destroy(HeavyPacket *hp);

#endif /* HEAVYPACKET_H_ */
