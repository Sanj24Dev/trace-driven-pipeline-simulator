# trace-driven-pipeline-simulator
## Overview
This project involves implementing the CPU pipeline simulator with functionalities for a Reorder Buffer (ROB) and Register Alias Table (RAT). The simulator supports both in-order and out-of-order scheduling policies, with an extension for a 2-wide superscalar machine.

## Implementation Details

### Source Files
- pipeline.cpp: Contains pipeline functions (issue, schedule, writeback, and commit).
- pipeline.h: Header file containing essential structs and definitions for the simulator.
- sim.cpp: Handles trace file operations, initialization, and pipeline execution.
- rat.cpp & rat.h: Implement and define the Register Alias Table functionality.
- rob.cpp & rob.h: Implement and define the Reorder Buffer functionality.
- execq.cpp & execq.h: Provides execution functionality for the simulator.

### Scripts
- Makefile: Compilation and automation tool with the following commands:
- make or make all: Compile the entire project.
- make clean: Remove compiled files.
- make fast: Compile with the -O2 optimization flag.
- make debug: Compile with debugging enabled (DEBUG preprocessor directive).
- make profile: Compile for performance profiling with gprof.
- make validate: Validate output using runtests.sh.
- make runall: Run all traces using runall.sh.
- make submit: Create a tarball.

### Traces and Results
- traces/: Contains provided execution trace files.
- results/: Stores output from runall.sh.
- ref/: Reference outputs and reports for validating your implementation.

### Helper Scripts
- runall.sh: Executes all traces and generates a report (report.txt).
- runtests.sh: Runs a subset of traces and verifies output against reference results.

## Simulator Parameters
The simulator accepts the following command-line parameters:

- pipewidth <width>: Set pipeline width (default: 1).
- loadlatency <num>: Set load instruction latency (default: 4 cycles).
- schedpolicy <num>: Select scheduling policy:
0: In-order.
1: Out-of-order (default).
-h: Display usage information

```
./sim -pipewidth 2 -schedpolicy 1 -loadlatency 4 ../traces/sml.ptr.gz
```

### Simulator Statistics
- stat_retired_inst: Number of committed instructions.
- stat_num_cycle: Number of simulated CPU cycles.
- CPI (Cycles Per Instruction): Calculated as stat_num_cycle / stat_retired_inst.
