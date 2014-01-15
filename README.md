MCA^2
=====

MCA^2 standalone system implementation including compressed and non-compressed Aho-Corasick implementations.
This repository is based on the following papers:

___Pattern matching code, Aho-Corasick implementation, DFA compression:___

Anat Bremler-Barr, Yotam Harchol, David Hay: Space-time tradeoffs in software-based deep Packet Inspection. HPSR 2011: 1-8

___Smart multicore environment with heavy packet detection and offloading:___

Yehuda Afek, Anat Bremler-Barr, Yotam Harchol, David Hay, Yaron Koral: MCA2: multi-core architecture for mitigating complexity attacks. ANCS 2012: 235-246

Usage:
------

__Input files__

You should have two input files to run the non-compressed AC:
- Patterns file
- Trace file

(unfortunately, it expects some special binary format defined for these files back then...)

_Patterns_

There is a ready-to-use patterns file in the ZIP, named SnortPatternsFull2.bin. It contains about 6K patterns taken from Snort at some point of time. If it's enough for you, use it. Otherwise, you should use the utils/SnortConverter java utility, which reads patterns in ASCII format (where non-ascii binary values are specified in |XX| format as in Snort), and creates a file in my special binary format.

_Trace_

There is some ugly (but correct) way to convert PCAP to this format, using the Java code in utils/DumpConverter. Compile DumpConverter.java and run it with no arguments, it will show you the way (using tcpdump to write the pcap in hexa, then running this utility to convert the hexa to binary...).

__Running Pattern Matching (HPSR'11)__

To run the executable (say it is called main) you need to specify some arguments:
```
-t will time the run and show throughput
-m:X will use X threads for DPI
-a:path will read patterns from the given path and build a non-compressed AC DFA to scan with
-s:path will scan the trace given in the path
-c:path will read patterns and create a compressed automaton from them. It should be used with -l:0 -b:0 -d:prefix_path where prefix_path is a path to some file prefix that will be used to create several files that together represent the compressed automaton.
(you cannot use -c with -a or with -s)
-r:prefix_path will read a compressed automaton to scan with, to use with -s
```
Example run of non-compressed AC DFA:
```
./main -t -m:1 -a:SnortPatternsFull2.bin -s:my_trace.bin
```
Example run of compressed AC automaton:

Step 1: create the compressed automaton and save to disk
```
./main -c:SnortPatternsFull2.bin -d:snort_compressed
```
(this creates three files: snort_compressed.lookup, snort_compressed.patterns, snort_compressed.states)

Step 2: run the compressed AC automaton on a trace:
```
./main -t -m:1 -r:snort_compressed -s:my_trace.bin
```

__Running MCA^2 (ANCS'12)__

To run the system in MCA^2 mode (multithreaded with heavy packet isolation and transfer), uncomment the HYBRID_SCANNER flag in Common/Flags.h, compile, and run with both -a and -r, along with -s. You should also specify more parameters as appears in the usage string printed when running with no parameters.

For questions, contact yotamhc
