// rob.h
// Declares the struct for the re-order buffer.

#ifndef _ROB_H_
#define _ROB_H_

#include "trace.h"
#include <inttypes.h>

/**
 * The maximum allowed number of ROB entries.
 */
#define MAX_ROB_ENTRIES 256

/** A single entry of the ROB that can hold one instruction. */
typedef struct ROBEntryStruct
{
    /**
     * If the enrty containa a valid instruction
     */
    bool valid;

    /** If the instruction has started executing */
    bool exec;

    /**
     * If the instruction's output ready? 
     */
    bool ready;

    /**
     * The instruction that this entry holds.
     */
    InstInfo inst;
} ROBEntry;

/**
 * The re-order buffer.
 * 
 * The ROB should be used as a circular buffer: when the head or tail pointers
 * reach NUM_ROB_ENTRIES, they should be wrapped around to 0.
 */
typedef struct ROB
{
    /**
     * An array of entries in the ROB. Each can hold one instruction.
     */
    ROBEntry entries[MAX_ROB_ENTRIES];

    /**
     * The index of the head entry of the ROB
     */
    int head_ptr;

    /**
     * The index of the tail entry of the ROB
     */
    int tail_ptr;
} ROB;

/**
 * Allocate and initialize a new ROB
 * 
 * @return a pointer to a newly allocated ROB
 */
ROB *rob_init();

/**
 * Print out the state of the ROB for debugging purposes
 * 
 * @param rob the ROB
 */
void rob_print_state(ROB *rob);

/**
 * Check if there is space available to insert another instruction into the ROB
 * 
 * @param rob the ROB
 * @return true if the ROB has space for another instruction, false otherwise
 */
bool rob_check_space(ROB *rob);

/**
 * Insert an instruction into the ROB at the tail pointer
 * 
 * @param rob the ROB
 * @param inst the instruction to insert
 * @return the ID (index) of the newly inserted instruction in the ROB, or -1
 *         if there is no more space in the ROB
 */
int rob_insert(ROB *rob, InstInfo inst);

/**
 * Find the given instruction in the ROB and mark it as executing
 * 
 * @param rob the ROB
 * @param inst the instruction that is now executing
 */
void rob_mark_exec(ROB *rob, InstInfo inst);

/**
 * Find the given instruction in the ROB and mark it as having its output ready (i.e., being ready to commit)
 * 
 * @param rob the ROB
 * @param inst the instruction whose output is ready
 */
void rob_mark_ready(ROB *rob, InstInfo inst);

/**
 * Check if the instruction with the given tag (ID/index) has its output ready
 * 
 * @param rob the ROB
 * @param tag the tag (ID/index) of the instruction to check
 * @return true if the instruction has its output ready, false if it is not or
 *         if there is no valid instruction at this tag in the ROB
 */
bool rob_check_ready(ROB *rob, int tag);

/**
 * Check if the instruction at the head of the ROB is ready to commit
 * 
 * @param rob the ROB
 * @return true if the instruction at the head of the ROB is ready to commit,
 *         false if it is not or if there is no valid instruction at the head
 *         of the ROB
 */
bool rob_check_head(ROB *rob);

/**
 * Wake up instructions that are dependent on the instruction with the given tag
 * 
 * @param rob the ROB
 * @param tag the tag of the instruction that has finished executing
 */
void rob_wakeup(ROB *rob, int tag);

/**
 * If the head entry of the ROB is ready to commit, remove that entry and
 * return the instruction contained there
 * 
 * @param rob the ROB
 * @return the instruction that was previously at the head of the ROB
 */
InstInfo rob_remove_head(ROB *rob);

#endif
