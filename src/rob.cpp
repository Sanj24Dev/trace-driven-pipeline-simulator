// rob.cpp

////////////////////////////////////////////////////////////////////////
// Contains the following implementation to simulate re-order buffer: //
// - bool rob_check_space(ROB *rob)                                   //
// - int rob_insert(ROB *rob, InstInfo inst)                          //
// - void rob_mark_exec(ROB *rob, InstInfo inst)                      //
// - void rob_mark_ready(ROB *rob, InstInfo inst)                     //
// - bool rob_check_ready(ROB *rob, int tag)                          //
// - bool rob_check_head(ROB *rob)                                    //
// - void rob_wakeup(ROB *rob, int tag)                               //
// - InstInfo rob_remove_head(ROB *rob)                               //
////////////////////////////////////////////////////////////////////////


#include "rob.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * The number of entries in the ROB; that is, the maximum number of
 * instructions that can be stored in the ROB at any given time.
 */
extern uint32_t NUM_ROB_ENTRIES;

/**
 * Allocate and initialize a new ROB
 * 
 * @return a pointer to a newly allocated ROB
 */
ROB *rob_init()
{
    ROB *rob = (ROB *)calloc(1, sizeof(ROB));

    rob->head_ptr = 0;
    rob->tail_ptr = 0;

    for (int i = 0; i < MAX_ROB_ENTRIES; i++)
    {
        rob->entries[i].valid = false;
        rob->entries[i].ready = false;
        rob->entries[i].exec = false;
    }

    return rob;
}

/**
 * Print out the state of the ROB for debugging purposes
 * 
 * @param rob the ROB
 */
void rob_print_state(ROB *rob)
{
    printf("Current ROB state:\n");
    printf("Entry\t\tInst\tValid\tExec\tReady\tsrc1_reg\tsrc1_ready\tsrc1_tag\tsrc2_reg\tsrc2_ready\tsrc2_tag\tdest_reg\tdr_tag\n");
    for (unsigned int i = 0; i < NUM_ROB_ENTRIES; i++)
    {
        printf("%5d ::  %5d", i, (int)rob->entries[i].inst.inst_num);
        printf(" %5d", rob->entries[i].valid);
        printf(" %7d", rob->entries[i].exec);
        printf(" %7d", rob->entries[i].ready);
        printf(" %8d", rob->entries[i].inst.src1_reg);
        printf(" %10d", rob->entries[i].inst.src1_ready);
        printf(" %12d", rob->entries[i].inst.src1_tag);
        printf(" %11d", rob->entries[i].inst.src2_reg);
        printf(" %10d", rob->entries[i].inst.src2_ready);
        printf(" %12d", rob->entries[i].inst.src2_tag);
        printf(" %11d", rob->entries[i].inst.dest_reg);
        printf(" %10d", rob->entries[i].inst.dr_tag);
        printf(" %10d", rob->entries[i].inst.op_type);
        if (i == (unsigned int) rob->head_ptr && i == (unsigned int) rob->tail_ptr) {
            printf(" (head/tail)");
        } else if (i == (unsigned int) rob->head_ptr) {
            printf(" (head)");
        } else if (i == (unsigned int) rob->tail_ptr) {
            printf(" (tail)");
        } 
        printf("\n");
    }
    printf("\n");
}

/**
 * Check if there is space available to insert another instruction into the ROB
 * 
 * @param rob the ROB
 * @return true if the ROB has space for another instruction, false otherwise
 */
bool rob_check_space(ROB *rob)
{
    // Return true if there is space to insert another instruction into the ROB, false otherwise.
    if ((unsigned int)rob->tail_ptr == (unsigned int)rob->head_ptr) 
    {
        if (rob->entries[rob->head_ptr].valid == false)
        {
            return true;
        }
        else 
        {
            return false;
        }
    }
    else 
    {
        return true;
    }
}

/**
 * Insert an instruction into the ROB at the tail pointer
 * 
 * @param rob the ROB
 * @param inst the instruction to insert
 * @return the ID (index) of the newly inserted instruction in the ROB, or -1
 *         if there is no more space in the ROB
 */
int rob_insert(ROB *rob, InstInfo inst)
{
    // Check if there is space available in the ROB
    if (rob_check_space(rob)) 
    {
        // Create an entry
        int idx = rob->tail_ptr;
        rob->entries[idx].valid = true;
        rob->entries[idx].exec = false;
        rob->entries[idx].ready = false;
        rob->entries[idx].inst = inst;
        rob->tail_ptr = (rob->tail_ptr + 1) % NUM_ROB_ENTRIES;
        return idx;
    }
    else 
    {
        return -1;
    }
}

/**
 * Find the given instruction in the ROB and mark it as executing
 * 
 * @param rob the ROB
 * @param inst the instruction that is now executing
 */
void rob_mark_exec(ROB *rob, InstInfo inst)
{
    // Update rob entry containing the given instruction
    rob->entries[inst.dr_tag].exec = true;
}

/**
 * Find the given instruction in the ROB and mark it as having its output ready
 * 
 * @param rob the ROB
 * @param inst the instruction whose output is ready
 */
void rob_mark_ready(ROB *rob, InstInfo inst)
{
    // Update rob entry containing the given instruction
    rob->entries[inst.dr_tag].ready = true;
}

/**
 * Check if the instruction with the given tag (ID/index) has its output ready
 * 
 * @param rob the ROB
 * @param tag the tag (ID/index) of the instruction to check
 * @return true if the instruction has its output ready, false if it is not or
 *         if there is no valid instruction at this tag in the ROB
 */
bool rob_check_ready(ROB *rob, int tag)
{
    // Return true if the instruction at this tag (ID/index) is valid and
    //       has its output ready (i.e., is ready to commit), false otherwise.
    if (rob->entries[tag].valid && rob->entries[tag].ready) 
    {
        return true;
    }
    else 
    {
        return false;
    }
}

/**
 * Check if the instruction at the head of the ROB is ready to commit
 * 
 * @param rob the ROB
 * @return true if the instruction at the head of the ROB is ready to commit,
 *         false if it is not or if there is no valid instruction at the head
 *         of the ROB
 */
bool rob_check_head(ROB *rob)
{
    // Return true if the instruction at the head of the ROB is valid and
    //       ready to commit, false otherwise.
    if (rob_check_ready(rob, rob->head_ptr)) 
    {
        return true;
    }
    else 
    {
        return false;
    }
}

/**
 * Wake up instructions that are dependent on the instruction with the given tag
 * 
 * @param rob the ROB
 * @param tag the tag of the instruction that has finished executing
 */
void rob_wakeup(ROB *rob, int tag)
{
    unsigned int tail= rob->tail_ptr, head = rob->head_ptr;
    unsigned int i = head;
    do
    {
        if (rob->entries[i].valid && rob->entries[i].inst.src1_tag == tag) 
        {
            // Update the src1 ready bits
            rob->entries[i].inst.src1_ready = true;
        }
        if (rob->entries[i].valid && rob->entries[i].inst.src2_tag == tag) 
        {
            // Update the src2 ready bits
            rob->entries[i].inst.src2_ready = true;
        }
        i = (i+1)%NUM_ROB_ENTRIES;
    } while( i != tail );
}

/**
 * If the head entry of the ROB is ready to commit, remove that entry and
 * return the instruction contained there
 * 
 * @param rob the ROB
 * @return the instruction that was previously at the head of the ROB
 */
InstInfo rob_remove_head(ROB *rob)
{
    InstInfo headEntry;
    // Check if the head entry is ready to commit
    if(rob->entries[rob->head_ptr].valid && rob->entries[rob->head_ptr].ready) 
    {
        // Remove that entry
        headEntry = rob->entries[rob->head_ptr].inst;
        rob->entries[rob->head_ptr].valid = false;
        rob->entries[rob->head_ptr].exec = false;
        rob->entries[rob->head_ptr].ready = false;
        rob->head_ptr = (rob->head_ptr + 1) % NUM_ROB_ENTRIES;
    }
    return headEntry;
}
