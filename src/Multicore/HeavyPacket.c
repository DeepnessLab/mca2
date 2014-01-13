/*
 * HeavyPacket.c
 *
 *  Created on: Dec 20, 2011
 *      Author: yotam
 */

#include <stdlib.h>
#include "HeavyPacket.h"

HeavyPacket *heavy_packet_create(Packet *p, unsigned int last_idx_in_root) {
	HeavyPacket *hp;

	hp = (HeavyPacket*)malloc(sizeof(HeavyPacket));
	hp->packet = p;
	hp->last_idx_in_root = last_idx_in_root;

	return hp;
}

void heavy_packet_destroy(HeavyPacket *hp) {
	free(hp);
}
