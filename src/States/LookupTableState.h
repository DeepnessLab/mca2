/*
 * LookupTableState.h
 *
 *  Created on: Jan 12, 2011
 *      Author: yotamhc
 */

#ifndef LOOKUPTABLESTATE_H_
#define LOOKUPTABLESTATE_H_

#include "../StateMachine/StateMachine.h"
#include "../Common/PatternTable.h"
#include "../Common/Types.h"

//State *createState_LT(int size, char *chars, STATE_PTR_TYPE *gotos, int *matches, STATE_PTR_TYPE failure);
int getNextState_LT(State *lookupTableState, char *str, int length, int *idx, NextStateResult *result, StateMachine *machine, PatternTable *patternTable, int verbose);
void printState_LT(State *state);
State *createEmptyState_LT(int id, int size, int numPtrsToFirstOrSecondLevel);
void setStateData_LT(State *state, uchar *chars, STATE_PTR_TYPE *gotos, short *ptrTypes, int *matches, STATE_PTR_TYPE failure);
STATE_PTR_TYPE getStateID_LT(State *state);
int getSizeInBytes_LT(State *state);
State *getNextStatePointer_LT(State *lookupTableState);
#ifdef COUNT_CALLS
void printCounter_LT();
#endif


// ******************

#define FAIL_IDX 5

#define GET_NEXT_STATE_LT(lookupTableState, str, length, idx, result, machine, patternTable, verbose)											\
	do {																																		\
																																				\
		uchar *table = &((lookupTableState)[FAIL_IDX + 2 + 32]);																				\
		uchar c = (uchar)str[*(idx)];																											\
		int s_idx = c * 2;	\
																																				\
		if (table[s_idx] != 0 || table[s_idx + 1] != 0) { /* There is a goto */																	\
			int i, count, ind;																													\
			(result)->next = GET_STATE_POINTER_FROM_ID(machine, (table[(int)s_idx + 1] << 8) | (table[(int)s_idx]));							\
			(result)->match = GET_1BIT_ELEMENT(&((lookupTableState)[FAIL_IDX + 2]), c);															\
			if ((verbose) && (result)->match) {																									\
				count = 0;																														\
				ind = FAIL_IDX + 2;																												\
				for (i = 0; i < 32; i++) {																										\
					if (((lookupTableState)[ind + i] & 0x1) != 0 && i * 8 < (int)c) count++;													\
					if (((lookupTableState)[ind + i] & 0x2) != 0 && i * 8 + 1 < (int)c) count++;												\
					if (((lookupTableState)[ind + i] & 0x4) != 0 && i * 8 + 2 < (int)c) count++;												\
					if (((lookupTableState)[ind + i] & 0x8) != 0 && i * 8 + 3 < (int)c) count++;												\
					if (((lookupTableState)[ind + i] & 0x16) != 0 && i * 8 + 4 < (int)c) count++;												\
					if (((lookupTableState)[ind + i] & 0x32) != 0 && i * 8 + 5 < (int)c) count++;												\
					if (((lookupTableState)[ind + i] & 0x64) != 0 && i * 8 + 6 < (int)c) count++;												\
					if (((lookupTableState)[ind + i] & 0x128) != 0 && i * 8 + 7 < (int)c) count++;												\
					if (i * 8 + 8 < (int)c)																										\
						break;																													\
				}																																\
				(result)->pattern = (patternTable)->patterns[((lookupTableState)[2] << 8) | (lookupTableState)[3]][count];						\
			}																																	\
			(result)->fail = FALSE;																												\
			(*idx)++;																															\
		} else { /* Failure */																													\
			(result)->next = GET_STATE_POINTER_FROM_ID(machine, ((lookupTableState)[FAIL_IDX + 1] << 8) | ((lookupTableState)[FAIL_IDX]));		\
			(result)->match = FALSE;																											\
			(result)->fail = TRUE;																												\
		}																																		\
	} while (FALSE);
// ******************

#endif /* LOOKUPTABLESTATE_H_ */
