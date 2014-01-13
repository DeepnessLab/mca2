/*
 * SimpleLinearEncodedState.h
 *
 *  Created on: Feb 7, 2011
 *      Author: yotamhc
 */

#ifndef SIMPLELINEARENCODEDSTATE_H_
#define SIMPLELINEARENCODEDSTATE_H_

#include "../StateMachine/StateMachine.h"
#include "../Common/PatternTable.h"
#include "../Common/Types.h"

//State *createState_LE(int size, char *chars, STATE_PTR_TYPE *gotos, int *matches, STATE_PTR_TYPE failure);
int getNextState_SLE(State *simpleLinearEncodedState, char *str, int length, int *idx, NextStateResult *result, StateMachine *machine, PatternTable *patternTable, int verbose);
State *createEmptyState_SLE(int id, int size);
void setStateData_SLE(State *state, uchar *chars, STATE_PTR_TYPE_WIDE *gotos, STATE_PTR_TYPE_WIDE failure, int match);

#endif /* SIMPLELINEARENCODEDSTATE_H_ */
