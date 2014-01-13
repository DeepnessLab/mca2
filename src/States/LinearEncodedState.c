/*
 * LinearEncodedState.c
 *
 *  Created on: Jan 11, 2011
 *      Author: yotamhc
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "../Common/FastLookup/FastLookup.h"
#include "../Common/BitArray/BitArray.h"
#include "LinearEncodedState.h"

#define FAIL_IDX 4
/*
State *createState_LE(int size, char *chars, STATE_PTR_TYPE *gotos, int *matches, STATE_PTR_TYPE failure) {
	int numPtrsToFirstOrSecondLevel = 0;
	int i, j, isptr, memsize;
	int current_idx_flags, current_idx_gotos;
	unsigned int flag;
	uchar *data;
	StateHeader header;

	header.type = STATE_TYPE_LINEAR_ENCODED;
	header.size = size;

	for (i = 0; i < size; i++) {
		if (isFirstLevelState(gotos[i]) || isSecondLevelState(gotos[i]))
			numPtrsToFirstOrSecondLevel++;
	}

	memsize = 3 + 1 + size * sizeof(char) +						// header + sizeInBytes + chars
			PTR_SIZE * (size - numPtrsToFirstOrSecondLevel) +	// real ptrs
			(int)ceil((3 * size) / 8.0) +				// two bits flags + match bits
			PTR_SIZE;											// fail ptr

	data = (unsigned char*)malloc(memsize);

	// [ type:2, size:6, id:16, sizeInBytesOfThisStruct, fail:16, char0, ..., charN-1, flag0, match0, ..., flagN-1, matchN-1, ptr_i, ..., ptr_j ]
	// sizeInBytesOfThisStruct - for debug purposes...

	data[0] = (uchar)(GET_HEADER_AS_BYTE0(header));
	data[1] = (uchar)(GET_HEADER_AS_BYTE1(header));
	data[2] = (uchar)(GET_HEADER_AS_BYTE2(header));
	data[3] = (uchar)memsize;
	data[FAIL_IDX] = (uchar)failure;
	data[FAIL_IDX + 1] = (uchar)(failure >> 8);

	current_idx_flags = FAIL_IDX + 2 + size;
	current_idx_gotos = FAIL_IDX + 2 + size + (int)ceil((3 * size) / 8.0);
	j = 0; // current goto pointer (<= i)
	isptr = FALSE;

	for (i = 0; i < size; i++) {
		// Set character
		data[FAIL_IDX + 2 + i] = chars[i];

		flag = PTR_TYPE_REGULAR;

		// Set two bits for pointer type
		isptr = FALSE;
		if (isFirstLevelState(gotos[i])) {
			flag = PTR_TYPE_LEVEL1;
		} else if (isSecondLevelState(gotos[i])) {
			flag = PTR_TYPE_LEVEL2;
		} else {
			isptr = TRUE;
		}

		// Set one bit for match
		flag <<= 1;
		flag = flag | matches[i];

		SET_3BITS_ELEMENT(&data[current_idx_flags], i, flag);

		if (isptr) {
			data[current_idx_gotos + j * 2] = (uchar)gotos[i];
			data[current_idx_gotos + j * 2 + 1] = (uchar)(gotos[i] >> 8);
			j++;
		}
	}

	return (State*)data;
}
*/
State *createEmptyState_LE(int id, int size, int numPtrsToFirstOrSecondLevel) {
	int memsize;
	uchar *data;
	StateHeader header;

	header.type = STATE_TYPE_LINEAR_ENCODED;
	header.size = size;

	memsize = 3 + 1 + size * sizeof(char) +						// header + sizeInBytes + chars
			PTR_SIZE * (size - numPtrsToFirstOrSecondLevel) +	// real ptrs
			(int)ceil((3 * size) / 8.0) +				// two bits flags + match bits
			PTR_SIZE;											// fail ptr

	data = (unsigned char*)malloc(memsize);

	// [ type:2, size:6, idMSB:8, idLSB:8, sizeInBytesOfThisStruct, fail:16, char0, ..., charN-1, flag0, match0, ..., flagN-1, matchN-1, ptr_i, ..., ptr_j ]
	// sizeInBytesOfThisStruct - for debug purposes...

	data[0] = (uchar)(GET_HEADER_AS_BYTE0(header));
	data[1] = (uchar)(id >> 8);
	data[2] = (uchar)id;
	data[3] = (uchar)memsize;
	return (State*)data;
}

void setStateData_LE(State *state, uchar *chars, STATE_PTR_TYPE *gotos, short *ptrTypes, int *matches, STATE_PTR_TYPE failure) {
	int i, j, isptr, size, memsize;
	int current_idx_flags, current_idx_gotos;
	unsigned int flag;
	uchar *data = (uchar*)state;
	StateHeader *header;

	header = (StateHeader*)state;
	size = header->size;
	memsize = state[3];
	data[FAIL_IDX] = (uchar)failure;
	data[FAIL_IDX + 1] = (uchar)(failure >> 8);

	current_idx_flags = FAIL_IDX + 2 + size;
	current_idx_gotos = FAIL_IDX + 2 + size + (int)ceil((3 * size) / 8.0);
	memset(&(data[current_idx_flags]), 0, (int)ceil((3 * size) / 8.0));

	j = 0; // current goto pointer (<= i)
	isptr = FALSE;

	for (i = 0; i < size; i++) {
		// Set character
		data[FAIL_IDX + 2 + i] = chars[i];

		flag = PTR_TYPE_REGULAR;

		// Set two bits for pointer type
		isptr = FALSE;
		/*
		if (isFirstLevelState(gotos[i])) {
			flag = PTR_TYPE_LEVEL1;
		} else if (isSecondLevelState(gotos[i])) {
			flag = PTR_TYPE_LEVEL2;
		} else {
			isptr = TRUE;
		}
		*/
		flag = ptrTypes[i];
		isptr = (flag == PTR_TYPE_REGULAR);

		// Set one bit for match
		flag <<= 1;
		flag = flag | matches[i];

		SET_3BITS_ELEMENT(&data[current_idx_flags], i, flag);

		if (isptr) {
			if (current_idx_gotos + j * 2 + 1 >= memsize) {
				fprintf(stderr, "Memory index out of bound: %d, mem size: %d\n", (current_idx_gotos + j * 2 + 1), memsize);
				exit(1);
			}
			data[current_idx_gotos + j * 2] = (uchar)gotos[i];
			data[current_idx_gotos + j * 2 + 1] = (uchar)(gotos[i] >> 8);
			j++;
		}
	}
}

int getNextState_LE(State *linearEncodedState, char *str, int length, int *idx, NextStateResult *result, StateMachine *machine, PatternTable *patternTable, int verbose) {
	uchar c;
	StateHeader *header;
	int i, size, found, j, count, id;
	STATE_PTR_TYPE fail;
	char *chars;
	unsigned int value, value2;

	header = (StateHeader*)linearEncodedState;
	size = header->size;
	c = (uchar)str[*idx];
	chars = (char*)(&(linearEncodedState[FAIL_IDX + 2]));

	found = FALSE;
	for (i = 0; i < size; i++) {
		if (c == chars[i]) {
			found = TRUE;
			break;
		}
	}

	if (found) {
		result->fail = FALSE;

		// Find the pointer

		// find the i-th three-bits sequence
		value = GET_3BITS_ELEMENT(&linearEncodedState[FAIL_IDX + 2 + size], i);

		result->match = GET_MATCH_BIT(value);
		if (verbose && result->match) {
			// Count pattern index
			count = 0;
			for (j = 0; j < i; j++) {
				value2 = GET_3BITS_ELEMENT(&linearEncodedState[FAIL_IDX + 2 + size], j);
				if (GET_MATCH_BIT(value2))
					count++;
			}
			id = (linearEncodedState[1] << 8) | linearEncodedState[2];
			result->pattern = patternTable->patterns[id][count];
		}

		count = 0;
		switch (GET_PTR_TYPE(value)) {
		case PTR_TYPE_REGULAR:
			// Search ptr in ptrs array
			for (j = 0; j < i; j++) {
				value = GET_3BITS_ELEMENT(&linearEncodedState[FAIL_IDX + 2 + size], j);
				if (GET_PTR_TYPE(value) == PTR_TYPE_REGULAR)
					count++;
			}
			// The pointer is in:
			j = FAIL_IDX + 2 + size + (int)ceil((3 * size) / 8.0) + count * 2;
			result->next = GET_STATE_POINTER_FROM_ID(machine, (linearEncodedState[j + 1] << 8) | (linearEncodedState[j]));
			break;
		case PTR_TYPE_LEVEL1:
			result->next = GET_STATE_POINTER_FROM_ID(machine, GET_FIRST_LEVEL_STATE(machine, c));
			break;
		case PTR_TYPE_LEVEL2:
			result->next = GET_STATE_POINTER_FROM_ID(machine, GET_SECOND_LEVEL_STATE(machine, str[*idx - 1], c));
			break;
		default:
			fprintf(stderr, "Unknown pointer type in a linear encoded state: %d\n", GET_PTR_TYPE(value));
			exit(1);
		}
		(*idx)++;
	} else {
		fail = (linearEncodedState[FAIL_IDX + 1] << 8) | (linearEncodedState[FAIL_IDX]);
		result->next = GET_STATE_POINTER_FROM_ID(machine, fail);
		result->match = FALSE;
		result->fail = TRUE;
	}
	return found;
}

void printState_LE(State *state) {
	StateHeader *header = (StateHeader*)state;
	int size = header->size;
	int id = (state[1] << 8) | state[2];
	int sizeInBytes = state[3];
	int fail = (state[FAIL_IDX + 1] << 8) | state[FAIL_IDX];
	int i;

	printf("Linear encoded state\n");
	printf("ID: %d\n", id);
	printf("Size in bytes: %d\n", sizeInBytes);
	printf("Failure state: %d\n", fail);

	char *chars = (char*)malloc(sizeof(char) * size);
	int *matches = (int*)malloc(sizeof(int) * size);
	int *gotos = (int*)malloc(sizeof(int) * size);

	uchar flag;
	int count, j;

	for (i = 0; i < size; i++) {
		chars[i] = state[FAIL_IDX + 2 + i];
		flag = GET_3BITS_ELEMENT(&state[FAIL_IDX + 2 + size], i);
		matches[i] = GET_MATCH_BIT(flag);

		count = 0;
		switch (GET_PTR_TYPE(flag)) {
		case PTR_TYPE_REGULAR:
			// Search ptr is ptrs array
			for (j = 0; j < i; j++) {
				flag = GET_3BITS_ELEMENT(&state[FAIL_IDX + 2 + size], j);
				if (GET_PTR_TYPE(flag) == PTR_TYPE_REGULAR)
					count++;
			}
			// The pointer is in:
			j = FAIL_IDX + 2 + size + (int)ceil((3 * size) / 8.0) + count * 2;
			gotos[i] = (state[j + 1] << 8) | (state[j]);
			break;
		case PTR_TYPE_LEVEL1:
			gotos[i] = -1; //getFirstLevelState(c);
			break;
		case PTR_TYPE_LEVEL2:
			gotos[i] = -2; //getSecondLevelState(chars[*idx - 1], c);
			break;
		}
	}

	for (i = 0; i < size; i++) {
		printf("Char: %c, match: %d, goto: %d\n", chars[i], matches[i], gotos[i]);
	}

	free(gotos);
	free(matches);
	free(chars);
}

STATE_PTR_TYPE getStateID_LE(State *state) {
	return (state[1] << 8) | state[2];
}

int getSizeInBytes_LE(State *state) {
	return state[3];
}

State *getNextStatePointer_LE(State *linearEncodedState) {
	return &(linearEncodedState[linearEncodedState[3]]);
}
