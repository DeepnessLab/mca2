/*
 * LinearEncodedState.h
 *
 *  Created on: Jan 11, 2011
 *      Author: yotamhc
 */

#ifndef LINEARENCODEDSTATE_H_
#define LINEARENCODEDSTATE_H_

#include "../StateMachine/StateMachine.h"
#include "../Common/PatternTable.h"
#include "../Common/Types.h"

//State *createState_LE(int size, char *chars, STATE_PTR_TYPE *gotos, int *matches, STATE_PTR_TYPE failure);
int getNextState_LE(State *linearEncodedState, char *str, int length, int *idx, NextStateResult *result, StateMachine *machine, PatternTable *patternTable, int verbose);
void printState_LE(State *state);
State *createEmptyState_LE(int id, int size, int numPtrsToFirstOrSecondLevel);
void setStateData_LE(State *state, uchar *chars, STATE_PTR_TYPE *gotos, short *ptrTypes, int *matches, STATE_PTR_TYPE failure);
STATE_PTR_TYPE getStateID_LE(State *state);
int getSizeInBytes_LE(State *state);
State *getNextStatePointer_LE(State *linearEncodedState);

#endif /* LINEARENCODEDSTATE_H_ */
