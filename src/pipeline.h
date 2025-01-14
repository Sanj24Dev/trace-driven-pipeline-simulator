// pipeline.h
// Declares the pipeline struct, as well as functions, data structures, and
// enums related to it.

#ifndef _PIPELINE_H_
#define _PIPELINE_H_

#include "trace.h"
#include "rat.h"
#include "rob.h"
#include "exeq.h"
#include <inttypes.h>

/**
 * The maximum allowed width of the pipeline.
 */
#define MAX_PIPE_WIDTH 8

/**
 * The maximum number of instructions that can be written back in a single
 * cycle.
 */
#define MAX_WRITEBACKS 256

/** How the pipeline schedules instructions for execution. */
typedef enum SchedulingPolicyEnum
{
    SCHED_IN_ORDER,     // Instructions are scheduled in-order.
    SCHED_OUT_OF_ORDER, // Instructions are scheduled out-of-order.
    NUM_SCHED_POLICIES
} SchedulingPolicy;

/**
 * One of the latches in the pipeline. Each one of these can contain one
 * instruction to be processed by the next pipeline stage.
 */
typedef struct PipelineLatchStruct
{
    /**
     * If the instruction is valid
     */
    bool valid;

    /**
     * If the instruction should be stalled
     */
    bool stall;

    /**
     * A structure containing information about this instruction, such as what
     * type of instruction it is, what registers it reads and writes, and so
     * on.
     */
    InstInfo inst;
} PipelineLatch;

/**
 * The data structure for an out-of-order pipelined processor.
 */
typedef struct Pipeline
{
    /**
     * The pipeline latch holding fetched instructions.
     */
    PipelineLatch FE_latch[MAX_PIPE_WIDTH];

    /**
     * The pipeline latch holding decoded instructions.
     */
    PipelineLatch ID_latch[MAX_PIPE_WIDTH];

    /**
     * The pipeline latch holding scheduled instructions.
     */
    PipelineLatch SC_latch[MAX_PIPE_WIDTH];

    /**
     * The pipeline latch holding instructions that have completed execution.
     */
    PipelineLatch EX_latch[MAX_WRITEBACKS];

    /**
     * The re-order buffer, containing instructions that have been issued but
     * have not yet been committed.
     */
    ROB *rob;

    /**
     * The register alias table, containing information on which architectural
     * registers are aliased to which instructions in the ROB.
     */
    RAT *rat;

    /**
     * The execution queue for instructions that take multiple cycles to execute.
     */
    EXEQ *exeq;

    /**
     * The total number of committed instructions.
     */
    uint64_t stat_retired_inst;

    /**
     * The total number of simulated CPU cycles.
     */
    uint64_t stat_num_cycle;

    /** [Internal] The file descriptor from which to read trace records. */
    int trace_fd;
    /** [Internal] The last inst_num assigned. */
    uint64_t last_inst_num;
    /** [Internal] The inst_num of the last instruction in the trace. */
    uint64_t halt_inst_num;
    /** [Internal] Whether the pipeline is done. */
    bool halt;
} Pipeline;

/**
 * Allocate and initialize a new pipeline.
 * 
 * @param trace_fd the file descriptor from which to read trace records
 * @return a pointer to a newly allocated pipeline
 */
Pipeline *pipe_init(int trace_fd);

/**
 * Simulate one cycle of all stages of a pipeline.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle(Pipeline *p);

/**
 * Simulate one cycle of the fetch stage of a pipeline.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_fetch(Pipeline *p);

/**
 * Simulate one cycle of the instruction decode stage of a pipeline.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_decode(Pipeline *p);

/**
 * Simulate one cycle of the issue stage of a pipeline: insert decoded
 * instructions into the ROB and perform register renaming.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_issue(Pipeline *p);

/**
 * Simulate one cycle of the scheduling stage of a pipeline: schedule
 * instructions to execute if they are ready.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_schedule(Pipeline *p);

/**
 * Simulate one cycle of the execute stage of a pipeline. This handles
 * instructions that take multiple cycles to execute.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_exe(Pipeline *p);

/**
 * Simulate one cycle of the writeback stage of a pipeline: update the ROB
 * with information from instructions that have finished executing.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_writeback(Pipeline *p);

/**
 * Simulate one cycle of the commit stage of a pipeline: commit instructions
 * in the ROB that are ready to commit.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_commit(Pipeline *p);

/**
 * Commit the given instruction.
 * 
 * This updates counters and flags on the pipeline.
 * 
 * @param p the pipeline to update.
 * @param inst the instruction to commit.
 */
void pipe_commit_inst(Pipeline *p, InstInfo inst);

/**
 * Print out the state of the pipeline for debugging purposes.
 * 
 * @param p the pipeline
 */
void pipe_print_state(Pipeline *p);

#endif
