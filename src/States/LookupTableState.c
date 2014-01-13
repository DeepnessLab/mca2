/*
 * LookupTableState.c
 *
 *  Created on: Jan 12, 2011
 *      Author: yotamhc
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "../Common/BitArray/BitArray.h"
#include "../Common/FastLookup/FastLookup.h"
#include "LookupTableState.h"

#define SIZE_IN_BYTES 3+1+1+32+PTR_SIZE+PTR_SIZE*256 // header + sizeInBytes + match-bitmap + failure + table

/*
State *createState_LT(int size, char *chars, STATE_PTR_TYPE *gotos, int *matches, STATE_PTR_TYPE failure) {
	int i, memsize;
	int current_idx_gotos;
	uchar *data, *matchmap;
	StateHeader header;

	header.type = STATE_TYPE_LOOKUP_TABLE;
	header.size = (size >> 8);

	memsize = SIZE_IN_BYTES;

	data = (unsigned char*)malloc(memsize);

	// [ type:2, sizeMSB:6, id:16, sizeLSB:8, sizeInBytes, failure:16, match-bitmap(32bytes), ptr_0, ..., ptr_255 ]
	// size is part of the header but is not really necessary
	// sizeInBytesOfThisStruct - for debug purposes...

	data[0] = (uchar)(GET_HEADER_AS_BYTE0(header));
	data[1] = (uchar)(GET_HEADER_AS_BYTE1(header));
	data[2] = (uchar)(GET_HEADER_AS_BYTE2(header));
	data[3] = (uchar)size;
	data[4] = (uchar)255;

	data[FAIL_IDX] = (uchar)failure;
	data[FAIL_IDX + 1] = (uchar)(failure >> 8);

	matchmap = &(data[FAIL_IDX + 2]);
	memset(matchmap, 0, 32);

	current_idx_gotos = FAIL_IDX + 2 + 32;

	memset(&(data[current_idx_gotos]), 0, 256 * PTR_SIZE);

	for (i = 0; i < size; i++) {
		// Set match values
		SET_1BIT_ELEMENT(matchmap, (uchar)chars[i], matches[i]);

		data[current_idx_gotos + (int)chars[i] * 2] = (uchar)gotos[i];
		data[current_idx_gotos + (int)chars[i] * 2 + 1] = (uchar)(gotos[i] >> 8);
	}

	return (State*)data;
}
*/
State *createEmptyState_LT(int id, int size, int numPtrsToFirstOrSecondLevel) {
	int memsize;
	uchar *data;
	StateHeader header;

	header.type = STATE_TYPE_LOOKUP_TABLE;
	header.size = (size >> 8);

	memsize = SIZE_IN_BYTES;

	data = (unsigned char*)malloc(memsize);

	// [ type:2, sizeMSB:6, sizeLSB:8, idMSB:8, idLSB:8, sizeInBytes, failure:16, match-bitmap(32bytes), ptr_0, ..., ptr_255 ]
	// sizeInBytesOfThisStruct - for debug purposes...

	data[0] = (uchar)(GET_HEADER_AS_BYTE0(header));
	data[1] = (uchar)size;
	data[2] = (uchar)(id >> 8);
	data[3] = (uchar)id;
	data[4] = (uchar)255;

	return (State*)data;
}

void setStateData_LT(State *state, uchar *chars, STATE_PTR_TYPE *gotos, short *ptrTypes, int *matches, STATE_PTR_TYPE failure) {
	int i, size;
	int current_idx_gotos;
	uchar *data, *matchmap;
	StateHeader *header;

	data = (uchar*)state;
	header = (StateHeader*)state;
	size = (header->size << 8) | data[1];

	data[FAIL_IDX] = (uchar)failure;
	data[FAIL_IDX + 1] = (uchar)(failure >> 8);

	matchmap = &(data[FAIL_IDX + 2]);
	memset(matchmap, 0, 32);

	current_idx_gotos = FAIL_IDX + 2 + 32;

	memset(&(data[current_idx_gotos]), 0, 256 * PTR_SIZE);

	for (i = 0; i < size; i++) {
		// Set match values
		SET_1BIT_ELEMENT(matchmap, (uchar)chars[i], matches[i]);

		if (current_idx_gotos + (int)chars[i] * 2 + 1 >= SIZE_IN_BYTES) {
			fprintf(stderr, "Memory index out of bound: %d, mem size: %d\n", (current_idx_gotos + (int)chars[i] * 2 + 1), SIZE_IN_BYTES);
			exit(1);
		}

		data[current_idx_gotos + (int)chars[i] * 2] = (uchar)gotos[i];
		data[current_idx_gotos + (int)chars[i] * 2 + 1] = (uchar)(gotos[i] >> 8);
	}

	//printState_LT(state);
}

#ifdef COUNT_CALLS
static int counter = 0;
static int counter_root = 0;
#endif

int getNextState_LT(State *lookupTableState, char *str, int length, int *idx, NextStateResult *result, StateMachine *machine, PatternTable *patternTable, int verbose) {
	int id;
	int i, count, ind;

#ifdef COUNT_CALLS
	counter++;
	if (((lookupTableState[2] << 8) | lookupTableState[3]) == 0)
		counter_root++;
#endif

	uchar *table = &(lookupTableState[FAIL_IDX + 2 + 32]);
	uchar c = (uchar)str[*idx];

	if (table[c * 2] != 0 || table[c * 2 + 1] != 0) {
		// There is a goto
		result->next = GET_STATE_POINTER_FROM_ID(machine, (table[(int)c * 2 + 1] << 8) | (table[(int)c * 2]));
		result->match = GET_1BIT_ELEMENT(&(lookupTableState[FAIL_IDX + 2]), c);
		if (verbose && result->match) {
			count = 0;
			ind = FAIL_IDX + 2;
			for (i = 0; i < 32; i++) {
				if ((lookupTableState[ind + i] & 0x1) != 0 && i * 8 < (int)c) count++;
				if ((lookupTableState[ind + i] & 0x2) != 0 && i * 8 + 1 < (int)c) count++;
				if ((lookupTableState[ind + i] & 0x4) != 0 && i * 8 + 2 < (int)c) count++;
				if ((lookupTableState[ind + i] & 0x8) != 0 && i * 8 + 3 < (int)c) count++;
				if ((lookupTableState[ind + i] & 0x16) != 0 && i * 8 + 4 < (int)c) count++;
				if ((lookupTableState[ind + i] & 0x32) != 0 && i * 8 + 5 < (int)c) count++;
				if ((lookupTableState[ind + i] & 0x64) != 0 && i * 8 + 6 < (int)c) count++;
				if ((lookupTableState[ind + i] & 0x128) != 0 && i * 8 + 7 < (int)c) count++;
				if (i * 8 + 8 < (int)c)
					break;
			}
			id = (lookupTableState[2] << 8) | lookupTableState[3];
			result->pattern = patternTable->patterns[id][count];
		}
		result->fail = FALSE;
		(*idx)++;
		return TRUE;
	} else {
		// Failure
		result->next = GET_STATE_POINTER_FROM_ID(machine, (lookupTableState[FAIL_IDX + 1] << 8) | (lookupTableState[FAIL_IDX]));
		result->match = FALSE;
		result->fail = TRUE;
		return FALSE;
	}
}

void printState_LT(State *state) {
	//StateHeader *header;
	int size;
	int fail = (state[FAIL_IDX + 1] << 8) | state[FAIL_IDX];
	int i, id;
	uchar low, high;
	id = (state[2] << 8) | state[3];

	printf("Lookup table state\n");
	printf("ID: %d\n", id);
	printf("Size in bytes: %d\n", SIZE_IN_BYTES);
	printf("Failure state: %d\n", fail);

	size = 0;
	for (i = 0; i < 256; i++) {
		low = state[FAIL_IDX + 2 + 32 + i * 2];
		high = state[FAIL_IDX + 2 + 32 + i * 2 + 1];
		if (low != 0 || high != 0) {
			size++;
		}
	}
	printf("Real size: %d\n", size);

	char *chars = (char*)malloc(sizeof(char) * size);
	int *matches = (int*)malloc(sizeof(int) * size);
	int *gotos = (int*)malloc(sizeof(int) * size);
	uchar *bitmap = &(state[FAIL_IDX + 2]);

	int j;

	j = 0;
	for (i = 0; i < 256 && j < size; i++) {
		low = state[FAIL_IDX + 2 + 32 + i * 2];
		high = state[FAIL_IDX + 2 + 32 + i * 2 + 1];
		if (low != 0 || high != 0) {
			chars[j] = (char)i;
			matches[j] = GET_1BIT_ELEMENT(bitmap, i);
			gotos[j] = low | ((int)high << 8);
			j++;
		}
	}

	for (i = 0; i < size; i++) {
		printf("Char: %c, match: %d, goto: %d\n", chars[i], matches[i], gotos[i]);
	}

	free(gotos);
	free(matches);
	free(chars);
}

STATE_PTR_TYPE getStateID_LT(State *state) {
	return (state[2] << 8) | state[3];
}

int getSizeInBytes_LT(State *state) {
	return SIZE_IN_BYTES;
}

State *getNextStatePointer_LT(State *lookupTableState) {
	return &(lookupTableState[SIZE_IN_BYTES]);
}

#ifdef COUNT_CALLS
void printCounter_LT() {
	printf("getNextState_LT count: %d\n", counter);
	printf("getNextState_LT for ROOT count: %d\n", counter_root);
}
#endif
