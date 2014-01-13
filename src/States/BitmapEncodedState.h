/*
 * BitmapEncodedState.h
 *
 *  Created on: Jan 11, 2011
 *      Author: yotamhc
 */

#ifndef BITMAPENCODEDSTATE_H_
#define BITMAPENCODEDSTATE_H_

#include "../StateMachine/StateMachine.h"
#include "../Common/PatternTable.h"
#include "../Common/Types.h"

// chars, gotos, matches are expected to be sorted by chars alphabetically order
//State *createState_BM(int size, char *chars, STATE_PTR_TYPE *gotos, int *matches, STATE_PTR_TYPE failure);
int getNextState_BM(State *bitmapEncodedState, char *str, int length, int *idx, NextStateResult *result, StateMachine *machine, PatternTable *patternTable, int verbose);
void printState_BM(State *state);
State *createEmptyState_BM(int id, int size, int numPtrsToFirstOrSecondLevel);
void setStateData_BM(State *state, uchar *chars, STATE_PTR_TYPE *gotos, short *ptrTypes, int *matches, STATE_PTR_TYPE failure);
STATE_PTR_TYPE getStateID_BM(State *state);
int getSizeInBytes_BM(State *state);
State *getNextStatePointer_BM(State *bitmapEncodedState);

#endif /* BITMAPENCODEDSTATE_H_ */
