/*
 * FastLookup.c
 *
 *  Created on: Jan 11, 2011
 *      Author: yotamhc
 */
#include "FastLookup.h"
/*
// Might not get inlined as it's in a separate file
inline STATE_PTR_TYPE getFirstLevelState(StateMachine *machine, char last) {
	return machine->firstLevelLookupTable[(int)last];
}

inline STATE_PTR_TYPE getSecondLevelState(StateMachine *machine, char previous, char last) {
	STATE_PTR_TYPE *res = hashmap_get(machine->secondLevelLookupHash, GET_SECOND_LEVEL_HASH_KEY(previous, last));
	return *res;
}

inline State *getStatePointerFromId(StateMachine *machine, int state) {
	return machine->states->table[state];
}

*/
