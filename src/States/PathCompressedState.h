/*
 * PathCompressedState.h
 *
 *  Created on: Jan 11, 2011
 *      Author: yotamhc
 */

#ifndef PATHCOMPRESSEDSTATE_H_
#define PATHCOMPRESSEDSTATE_H_

#include "../StateMachine/StateMachine.h"
#include "../Common/PatternTable.h"
#include "../Common/Types.h"

//State *createState_PC(int size, char *chars, STATE_PTR_TYPE *failures, int *matches, STATE_PTR_TYPE next);
int getNextState_PC(State *pathCompressedState, char *str, int slen, int *idx, NextStateResult *result, StateMachine *machine, PatternTable *patternTable, int verbose);
void printState_PC(State *state);
State *createEmptyState_PC(int id, int size, int numFailPtrsToFirstOrSecondLevel);
void setStateData_PC(State *state, uchar *chars, STATE_PTR_TYPE *failures, short *ptrTypes, int *matches, STATE_PTR_TYPE next);
STATE_PTR_TYPE getStateID_PC(State *state);
int getSizeInBytes_PC(State *state);
State *getNextStatePointer_PC(State *pathCompressedState);

#ifdef COUNT_CALLS
void printCounter_PC();
#endif

// *****************

#define GOTO_IDX 6

#define GET_NEXT_STATE_PC(pathCompressedState, str, slen, idx, result, machine, patternTable, verbose) 										\
	do {																																	\
		StateHeader *header;																												\
		int i, j, count, plength, failed, matched/*, id*/;																						\
		uchar *chars;																														\
		char *pattern;																														\
		uchar *matches, *flags;																												\
																																			\
		pattern = NULL;																														\
		header = (StateHeader*)(pathCompressedState);																						\
		plength = (header->size << 8) | ((pathCompressedState)[1] & 0x0FF);																	\
		chars = (uchar*)(&((pathCompressedState)[GOTO_IDX + 2]));																				\
		matches = &((pathCompressedState)[GOTO_IDX + 2 + plength]);																			\
		flags = &((pathCompressedState)[GOTO_IDX + 2 + plength + (int)ceil(plength / 8.0)]);													\
																																			\
		failed = FALSE;																														\
		matched = FALSE;																													\
		for (i = 0; i < plength; i++) {																										\
			if (*(idx) >= (slen)) {																												\
				break;																														\
			}																																\
			if ((uchar)(str)[*(idx)] != chars[i]) {																								\
				failed = TRUE;																												\
				break;																														\
			} else {																														\
				if (GET_1BIT_ELEMENT(matches, i)) {																							\
					/* It is a match */																										\
					matched = TRUE;																											\
																																			\
					if (verbose) {																											\
						/* Count pattern index */																							\
						count = 0;																											\
						for (j = 0; j < i; j++) {																							\
							if (GET_1BIT_ELEMENT(matches, i))																				\
								count++;																									\
						}																													\
						/* id = ((pathCompressedState)[2] << 8) | (pathCompressedState)[3];*/														\
						/*pattern = concat_strings_nofree(pattern, (patternTable)->patterns[id][count]);*/										\
					}																														\
				}																															\
				(*(idx)) = (*(idx)) + 1;																													\
			}																																\
		}																																	\
																																			\
		if (failed) {																														\
			switch (GET_2BITS_ELEMENT(flags, i)) {																							\
			case PTR_TYPE_REGULAR:																											\
				/* Search ptr is ptrs array */																								\
				count = 0;																													\
				for (j = 0; j < i; j++) {																									\
					if (GET_2BITS_ELEMENT(flags, j) == PTR_TYPE_REGULAR)																	\
						count++;																											\
				}																															\
				/* The pointer is in: */																									\
				j = GOTO_IDX + 2 + plength + (int)ceil(plength / 8.0) + (int)ceil(plength / 4.0) + count * 2;									\
				(result)->next = GET_STATE_POINTER_FROM_ID(machine, ((pathCompressedState)[j + 1] << 8) | ((pathCompressedState)[j]));			\
				break;																														\
			case PTR_TYPE_LEVEL0:																											\
				(result)->next = (machine)->states->table[0];																					\
				break;																														\
			case PTR_TYPE_LEVEL1:																											\
				(result)->next = GET_STATE_POINTER_FROM_ID(machine, GET_FIRST_LEVEL_STATE(machine, (str)[*(idx) - 1]));							\
				break;																														\
			case PTR_TYPE_LEVEL2:																											\
				(result)->next = GET_STATE_POINTER_FROM_ID(machine, GET_SECOND_LEVEL_STATE(machine, (str)[*(idx) - 2], (str)[*(idx) - 1]));			\
				break;																														\
			default:																														\
				fprintf(stderr, "Unknown pointer type in a path compressed state: %d\n", GET_2BITS_ELEMENT(flags, i));								\
				exit(1);																													\
			}																																\
			(result)->fail = TRUE;																											\
			(result)->match = matched;																										\
			(result)->pattern = pattern;																									\
		} else {																															\
			(result)->next = GET_STATE_POINTER_FROM_ID(machine, ((pathCompressedState)[GOTO_IDX + 1] << 8) | ((pathCompressedState)[GOTO_IDX]));\
			(result)->match = matched;																											\
			(result)->fail = FALSE;																												\
			(result)->pattern = pattern;																										\
		} 																																		\
	} while (FALSE);

#endif /* PATHCOMPRESSEDSTATE_H_ */
