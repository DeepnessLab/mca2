ifndef NO_OPTIMIZE
	O_SYM := -O3
else 
	O_SYM := -g
endif

LIBS := -lm -lpthread

all: main

clean: 
	rm *.o main

# EXECUTABLES
main: ACBuilder.o NodeQueue.o BitArray.o FastLookup.o HashMap.o PatternTable.o StateTable.o LinkedList.o PacketReader.o MulticoreManager.o Scanner.o CyclicLockFreeQueue.o GlobalTimer.o Timer.o DumpReader.o StateMachine.o StateMachineGenerator.o TableStateMachine.o TableStateMachineGenerator.o StateGenerator.o StateMachineDumper.o LinearEncodedState.o SimpleLinearEncodedState.o BitmapEncodedState.o LookupTableState.o PathCompressedState.o
	gcc -Wall $(O_SYM) -o main ../Main.c ACBuilder.o NodeQueue.o BitArray.o FastLookup.o HashMap.o PatternTable.o StateTable.o LinkedList.o PacketReader.o MulticoreManager.o Scanner.o CyclicLockFreeQueue.o GlobalTimer.o Timer.o DumpReader.o StateMachine.o StateMachineGenerator.o TableStateMachine.o TableStateMachineGenerator.o StateGenerator.o StateMachineDumper.o LinearEncodedState.o SimpleLinearEncodedState.o BitmapEncodedState.o LookupTableState.o PathCompressedState.o -lpthread -lm

# OBJECTS

ACBuilder.o: ../AhoCorasick/ACBuilder.c ../AhoCorasick/ACBuilder.h
	gcc -Wall $(O_SYM) -c ../AhoCorasick/ACBuilder.c -I../

NodeQueue.o: ../AhoCorasick/NodeQueue.c ../AhoCorasick/NodeQueue.h
	gcc -Wall $(O_SYM) -c ../AhoCorasick/NodeQueue.c -I../

BitArray.o: ../Common/BitArray/BitArray.c ../Common/BitArray/BitArray.h
	gcc -Wall $(O_SYM) -c ../Common/BitArray/BitArray.c -I../

FastLookup.o: ../Common/FastLookup/FastLookup.c ../Common/FastLookup/FastLookup.h
	gcc -Wall $(O_SYM) -c ../Common/FastLookup/FastLookup.c -I../

HashMap.o: ../Common/HashMap/HashMap.c ../Common/HashMap/HashMap.h
	gcc -Wall $(O_SYM) -c ../Common/HashMap/HashMap.c -I../

PatternTable.o: ../Common/PatternTable.c ../Common/PatternTable.h
	gcc -Wall $(O_SYM) -c ../Common/PatternTable.c -I../

StateTable.o: ../Common/StateTable.c ../Common/StateTable.h
	gcc -Wall $(O_SYM) -c ../Common/StateTable.c -I../

LinkedList.o: ../DumpReader/BoundedBuffer/LinkedList.c ../DumpReader/BoundedBuffer/LinkedList.h
	gcc -Wall $(O_SYM) -c ../DumpReader/BoundedBuffer/LinkedList.c -I../

PacketReader.o: ../DumpReader/BoundedBuffer/PacketReader.c ../DumpReader/BoundedBuffer/PacketReader.h
	gcc -Wall $(O_SYM) -c ../DumpReader/BoundedBuffer/PacketReader.c -I../

MulticoreManager.o: ../Multicore/MulticoreManager.c ../Multicore/MulticoreManager.h ../DumpReader/BoundedBuffer/Scanner.h  
	gcc -Wall $(O_SYM) -c ../Multicore/MulticoreManager.c -I../

Scanner.o: ../DumpReader/BoundedBuffer/Scanner.c ../DumpReader/BoundedBuffer/Scanner.h
	gcc -Wall $(O_SYM) -c ../DumpReader/BoundedBuffer/Scanner.c -I../

CyclicLockFreeQueue.o: ../Multicore/CyclicLockFreeQueue.c ../Multicore/CyclicLockFreeQueue.h ../DumpReader/DumpReader.h
	gcc -Wall $(O_SYM) -c ../Multicore/CyclicLockFreeQueue.c -I../

GlobalTimer.o: ../Common/GlobalTimer.c ../Common/GlobalTimer.h
	gcc -Wall $(O_SYM) -c ../Common/GlobalTimer.c -I../

Timer.o: ../Common/Timer.c ../Common/Timer.h
	gcc -Wall $(O_SYM) -c ../Common/Timer.c -I../

DumpReader.o: ../DumpReader/DumpReader.c ../DumpReader/DumpReader.h
	gcc -Wall $(O_SYM) -c ../DumpReader/DumpReader.c -I../

StateMachine.o: ../StateMachine/StateMachine.c ../StateMachine/StateMachine.h States
	gcc -Wall $(O_SYM) -c ../StateMachine/StateMachine.c -I../

StateMachineGenerator.o: ../StateMachine/StateMachineGenerator.c ../StateMachine/StateMachineGenerator.h
	gcc -Wall $(O_SYM) -c ../StateMachine/StateMachineGenerator.c -I../

TableStateMachine.o: ../StateMachine/TableStateMachine.c ../StateMachine/TableStateMachine.h States
	gcc -Wall $(O_SYM) -c ../StateMachine/TableStateMachine.c -I../

TableStateMachineGenerator.o: ../StateMachine/TableStateMachineGenerator.c ../StateMachine/TableStateMachineGenerator.h
	gcc -Wall $(O_SYM) -c ../StateMachine/TableStateMachineGenerator.c -I../

StateGenerator.o: ../StateMachine/StateGenerator.c ../StateMachine/StateGenerator.h States
	gcc -Wall $(O_SYM) -c ../StateMachine/StateGenerator.c -I../

StateMachineDumper.o: ../StateMachine/StateMachineDumper.c ../StateMachine/StateMachineDumper.h
	gcc -Wall $(O_SYM) -c ../StateMachine/StateMachineDumper.c -I../

States: LinearEncodedState.o BitmapEncodedState.o LookupTableState.o PathCompressedState.o SimpleLinearEncodedState.o

LinearEncodedState.o: ../States/LinearEncodedState.c ../States/LinearEncodedState.h
	gcc -Wall $(O_SYM) -c ../States/LinearEncodedState.c -I../

SimpleLinearEncodedState.o: ../States/SimpleLinearEncodedState.c ../States/SimpleLinearEncodedState.h
	gcc -Wall $(O_SYM) -c ../States/SimpleLinearEncodedState.c -I../

BitmapEncodedState.o: ../States/BitmapEncodedState.c ../States/BitmapEncodedState.h
	gcc -Wall $(O_SYM) -c ../States/BitmapEncodedState.c -I../ -msse4.2

LookupTableState.o: ../States/LookupTableState.c ../States/LookupTableState.h
	gcc -Wall $(O_SYM) -c ../States/LookupTableState.c -I../

PathCompressedState.o: ../States/PathCompressedState.c ../States/PathCompressedState.h
	gcc -Wall $(O_SYM) -c ../States/PathCompressedState.c -I../
