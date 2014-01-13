/*
 * SimpleLinearEncodedState.c
 *
 *  Created on: Jan 11, 2011
 *      Author: yotamhc
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "../Common/FastLookup/FastLookup.h"
#include "../Common/BitArray/BitArray.h"
#include "SimpleLinearEncodedState.h"

#define FAIL_IDX 6

#define MEM_SIZE_SIMPLE_TABLE_STATE

State *createEmptyTableState(int id, int size) {
	int memsize;
	uchar *data;
	StateHeader header;

	header.type = STATE_TYPE_LOOKUP_TABLE;
	header.size = 0; // NOT USED FOR SIZE BUT FOR: MATCH BIT, ID HIGH BIT, FAIL PTR MSBs (4)

	memsize = 1 + 1 + 2 + 2 + 256 * PTR_SIZE +				// header=(type:2, match:1, idMSB:1, failMSB:4) + size + sizeinbytes:16 + id + gotos
			32 + 32 +											// high bit of ptr, second high bit
			PTR_SIZE;											// fail ptr

	data = (unsigned char*)malloc(memsize);

	// [ type:2, size:6, idMSB:8, idLSB:8, sizeInBytesOfThisStruct, fail:16, char0, ..., charN-1, flag0, match0, ..., flagN-1, matchN-1, ptr_i, ..., ptr_j ]
	// sizeInBytesOfThisStruct - for debug purposes...

	header.size = (id > 0xFFFF) ? 0x10 : 0;

	data[0] = (uchar)(GET_HEADER_AS_BYTE0(header));
	data[1] = (uchar)size;
	data[2] = (uchar)(id >> 8);
	data[3] = (uchar)id;
	data[4] = (uchar)(memsize >> 8);
	data[5] = (uchar)memsize;
	return (State*)data;
}

void setTableStateData(State *state, uchar *chars, STATE_PTR_TYPE_WIDE *gotos, STATE_PTR_TYPE_WIDE failure, int match) {
	int i, j, size;
	int current_idx_flags, current_idx_gotos;
	uchar *data = (uchar*)state;
	StateHeader *header;

	header = (StateHeader*)state;
	size = data[1];
	data[FAIL_IDX] = (uchar)failure;
	data[FAIL_IDX + 1] = (uchar)(failure >> 8);
	header->size = (header->size) | (((uchar)(failure >> 16)) & 0x0F); // only 4 bits
	header->size = (header->size) | (match ? 0x20 : 0); // match bit

	current_idx_flags = FAIL_IDX + 2;
	current_idx_gotos = FAIL_IDX + 2 + 32 + 32;
	memset(&(data[current_idx_flags]), 0, 64);
	memset(&(data[current_idx_gotos]), 0xFF, 256 * PTR_SIZE);

	for (i = 0; i < size; i++) {
		// Set goto
		j = (int)(chars[i]);
		SET_1BIT_ELEMENT(&data[current_idx_flags], (j * 2), (gotos[i] >> 16));
		SET_1BIT_ELEMENT(&data[current_idx_flags], (j * 2 + 1), (gotos[i] >> 17));

		data[current_idx_gotos + j * 2] = (uchar)gotos[i];
		data[current_idx_gotos + j * 2 + 1] = (uchar)(gotos[i] >> 8);
	}
}

int getNextStateFromTableState(State *simpleTableEncodedState, char *str, int length, int *idx, NextStateResult *result, StateMachine *machine, PatternTable *patternTable, int verbose) {
	uchar c;
	StateHeader *header;
	int found, j;
	unsigned int fail;
	uchar *highbits, *ptrs;
	unsigned int value;

	header = (StateHeader*)simpleTableEncodedState;
	//size = simpleTableEncodedState[1];
	c = (uchar)str[*idx];
	highbits = (uchar*)(&(simpleTableEncodedState[FAIL_IDX + 2]));
	ptrs = (uchar*)(&(simpleTableEncodedState[FAIL_IDX + 2 + 64]));

	result->match = FALSE;

	value = ((GET_1BIT_ELEMENT(highbits, (c * 2)) & 0x01)) | ((GET_1BIT_ELEMENT(highbits, (c * 2 + 1)) & 0x01) << 1);

	if (value != 0x03 || ptrs[c * 2] == 0 || ptrs[c * 2 + 1] == 0) {
		result->fail = FALSE;

		// The pointer is in:
		j = FAIL_IDX + 2 + 32 + c * 2;
		result->next = GET_STATE_POINTER_FROM_ID(machine, (value << 16) | (simpleTableEncodedState[j + 1] << 8) | (simpleTableEncodedState[j]));

		(*idx)++;
		found = 1;
	} else {
		fail = ((header->size & 0x0F) << 16) | (simpleTableEncodedState[FAIL_IDX + 1] << 8) | (simpleTableEncodedState[FAIL_IDX]);
		result->next = GET_STATE_POINTER_FROM_ID(machine, fail);
		result->fail = TRUE;
		found = 0;
	}
	return found;
}

State *createEmptyState_SLE(int id, int size) {
	int memsize;
	uchar *data;
	StateHeader header;

#ifdef SIMPLE_USE_TABLE
	if (size >= 64) {
		return createEmptyTableState(id, size);
	}
#endif

	header.type = STATE_TYPE_LINEAR_ENCODED;
	header.size = 0; // NOT USED FOR SIZE BUT FOR: MATCH BIT, ID HIGH BIT, FAIL PTR MSBs (4)

	memsize = 1 + 1 + 2 + 2 + size * sizeof(char) +				// header=(type:2, match:1, idMSB:1, failMSB:4) + size + sizeinbytes:16 + id + chars
			PTR_SIZE * size +							// real ptrs
			(int)ceil((2 * size) / 8.0) +				// high/low ptr bit
			PTR_SIZE;											// fail ptr

	data = (unsigned char*)malloc(memsize);

	// [ type:2, size:6, idMSB:8, idLSB:8, sizeInBytesOfThisStruct, fail:16, char0, ..., charN-1, flag0, match0, ..., flagN-1, matchN-1, ptr_i, ..., ptr_j ]
	// sizeInBytesOfThisStruct - for debug purposes...

	header.size = (id > 0xFFFF) ? 0x10 : 0;

	data[0] = (uchar)(GET_HEADER_AS_BYTE0(header));
	data[1] = (uchar)size;
	data[2] = (uchar)(id >> 8);
	data[3] = (uchar)id;
	data[4] = (uchar)(memsize >> 8);
	data[5] = (uchar)memsize;
	return (State*)data;
}

void setStateData_SLE(State *state, uchar *chars, STATE_PTR_TYPE_WIDE *gotos, STATE_PTR_TYPE_WIDE failure, int match) {
	int i, size;
	int current_idx_flags, current_idx_gotos;
	uchar *data = (uchar*)state;
	StateHeader *header;

	header = (StateHeader*)state;

#ifdef SIMPLE_USE_TABLE
	if (header->type == STATE_TYPE_LOOKUP_TABLE) {
		setTableStateData(state, chars, gotos, failure, match);
		return;
	}
#endif

	size = data[1];
	data[FAIL_IDX] = (uchar)failure;
	data[FAIL_IDX + 1] = (uchar)(failure >> 8);
	header->size = (header->size) | (((uchar)(failure >> 16)) & 0x0F); // only 4 bits
	header->size = (header->size) | (match ? 0x20 : 0); // match bit

	current_idx_flags = FAIL_IDX + 2 + size;
	current_idx_gotos = FAIL_IDX + 2 + size + (int)ceil((2*size) / 8.0);
	memset(&(data[current_idx_flags]), 0, (int)ceil((2*size) / 8.0));

	for (i = 0; i < size; i++) {
		// Set character
		data[FAIL_IDX + 2 + i] = chars[i];

		SET_1BIT_ELEMENT(&data[current_idx_flags], (i * 2), (gotos[i] >> 16));
		SET_1BIT_ELEMENT(&data[current_idx_flags], (i * 2 + 1), (gotos[i] >> 17));

		data[current_idx_gotos + i * 2] = (uchar)gotos[i];
		data[current_idx_gotos + i * 2 + 1] = (uchar)(gotos[i] >> 8);
	}
}

int getNextState_SLE(State *simpleLinearEncodedState, char *str, int length, int *idx, NextStateResult *result, StateMachine *machine, PatternTable *patternTable, int verbose) {
	uchar c;
	StateHeader *header;
	int i, size, found, j;
	unsigned int fail;
	char *chars;
	unsigned int value;

	header = (StateHeader*)simpleLinearEncodedState;

#ifdef SIMPLE_USE_TABLE
	if (header->type == STATE_TYPE_LOOKUP_TABLE) {
		return getNextStateFromTableState(simpleLinearEncodedState, str, length, idx, result, machine, patternTable, verbose);
	}
#endif

	size = simpleLinearEncodedState[1];
	c = (uchar)str[*idx];
	chars = (char*)(&(simpleLinearEncodedState[FAIL_IDX + 2]));

	found = FALSE;
	for (i = 0; i < size; i++) {
		if (c == chars[i]) {
			found = TRUE;
			break;
		}
	}

	result->match = FALSE;

	if (found) {
		result->fail = FALSE;

		// Find the pointer

		// find the i-th high/low bit
		value = ((GET_1BIT_ELEMENT(&simpleLinearEncodedState[FAIL_IDX + 2 + size], (i * 2 + 1)) & 0x01) << 1) | (GET_1BIT_ELEMENT(&simpleLinearEncodedState[FAIL_IDX + 2 + size], (i * 2)) & 0x01);

		// The pointer is in:
		j = FAIL_IDX + 2 + size + (int)ceil((size) / 8.0) + i * 2;
		result->next = GET_STATE_POINTER_FROM_ID(machine, (value << 16) | (simpleLinearEncodedState[j + 1] << 8) | (simpleLinearEncodedState[j]));

		(*idx)++;
	} else {
		fail = ((header->size & 0x0F) << 16) | (simpleLinearEncodedState[FAIL_IDX + 1] << 8) | (simpleLinearEncodedState[FAIL_IDX]);
		result->next = GET_STATE_POINTER_FROM_ID(machine, fail);
		result->fail = TRUE;
	}
	return found;
}
