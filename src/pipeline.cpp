// pipeline.cpp
// Implements the out-of-order pipeline.

#include "pipeline.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * The width of the pipeline; that is, the maximum number of instructions that
 * can be processed during any given cycle in each of the issue, schedule, and
 * commit stages of the pipeline.
 * 
 * When the width is 1, the pipeline is scalar.
 * When the width is greater than 1, the pipeline is superscalar.
 */
extern uint32_t PIPE_WIDTH;

/**
 * The number of entries in the ROB; that is, the maximum number of
 * instructions that can be stored in the ROB at any given time.
 */
extern uint32_t NUM_ROB_ENTRIES;

/**
 * Whether to use in-order scheduling or out-of-order scheduling.
 * 
 * The possible values are SCHED_IN_ORDER for in-order scheduling and
 * SCHED_OUT_OF_ORDER for out-of-order scheduling.
 */
extern SchedulingPolicy SCHED_POLICY;

/**
 * The number of cycles an LD instruction should take to execute.
 */
extern uint32_t LOAD_EXE_CYCLES;

/**
 * Read a single trace record from the trace file and use it to populate the
 * given fe_latch.
 * 
 * @param p the pipeline whose trace file should be read
 * @param fe_latch the PipelineLatch struct to populate
 */
void pipe_fetch_inst(Pipeline *p, PipelineLatch *fe_latch)
{
    InstInfo *inst = &fe_latch->inst;
    TraceRec trace_rec;
    uint8_t *trace_rec_buf = (uint8_t *)&trace_rec;
    size_t bytes_read_total = 0;
    ssize_t bytes_read_last = 0;
    size_t bytes_left = sizeof(TraceRec);

    // Read a total of sizeof(TraceRec) bytes from the trace file.
    while (bytes_left > 0)
    {
        bytes_read_last = read(p->trace_fd, trace_rec_buf, bytes_left);
        if (bytes_read_last <= 0)
        {
            // EOF or error
            break;
        }

        trace_rec_buf += bytes_read_last;
        bytes_read_total += bytes_read_last;
        bytes_left -= bytes_read_last;
    }

    // Check for error conditions.
    if (bytes_left > 0 || trace_rec.op_type >= NUM_OP_TYPES)
    {
        fe_latch->valid = false;
        p->halt_inst_num = p->last_inst_num;

        if (p->stat_retired_inst >= p->halt_inst_num)
        {
            p->halt = true;
        }

        if (bytes_read_last == -1)
        {
            fprintf(stderr, "\n");
            perror("Couldn't read from pipe");
            return;
        }

        if (bytes_read_total == 0)
        {
            // No more trace records to read
            return;
        }

        // Too few bytes read or invalid op_type
        fprintf(stderr, "\n");
        fprintf(stderr, "Error: Invalid trace file\n");
        return;
    }

    // Got a valid trace record!
    fe_latch->valid = true;
    fe_latch->stall = false;
    inst->inst_num = ++p->last_inst_num;
    inst->op_type = (OpType)trace_rec.op_type;

    inst->dest_reg = trace_rec.dest_needed ? trace_rec.dest_reg : -1;
    inst->src1_reg = trace_rec.src1_needed ? trace_rec.src1_reg : -1;
    inst->src2_reg = trace_rec.src2_needed ? trace_rec.src2_reg : -1;

    inst->dr_tag = -1;
    inst->src1_tag = -1;
    inst->src2_tag = -1;
    inst->src1_ready = false;
    inst->src2_ready = false;
    inst->exe_wait_cycles = 0;
}

/**
 * Allocate and initialize a new pipeline.
 * 
 * @param trace_fd the file descriptor from which to read trace records
 * @return a pointer to a newly allocated pipeline
 */
Pipeline *pipe_init(int trace_fd)
{
    printf("\n** PIPELINE IS %d WIDE **\n\n", PIPE_WIDTH);

    // Allocate pipeline.
    Pipeline *p = (Pipeline *)calloc(1, sizeof(Pipeline));

    // Initialize pipeline.
    p->rat = rat_init();
    p->rob = rob_init();
    p->exeq = exeq_init();
    p->trace_fd = trace_fd;
    p->halt_inst_num = (uint64_t)(-1) - 3;

    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        p->FE_latch[i].valid = false;
        p->ID_latch[i].valid = false;
        p->SC_latch[i].valid = false;
    }
    for (unsigned int i = 0; i < MAX_WRITEBACKS; i++)
    {
        p->EX_latch[i].valid = false;
    }

    return p;
}

/**
 * Commit the given instruction.
 * 
 * This updates counters and flags on the pipeline.
 * 
 * @param p the pipeline to update.
 * @param inst the instruction to commit.
 */
void pipe_commit_inst(Pipeline *p, InstInfo inst)
{
    p->stat_retired_inst++;

    if (inst.inst_num >= p->halt_inst_num)
    {
        p->halt = true;
    }
}

/**
 * Print out the state of the pipeline for debugging purposes.
 * 
 * @param p the pipeline
 */
void pipe_print_state(Pipeline *p)
{
    printf("\n");
    // Print table header
    for (unsigned int latch_type = 0; latch_type < 4; latch_type++)
    {
        switch (latch_type)
        {
        case 0:
            printf(" FE:    ");
            break;
        case 1:
            printf(" ID:    ");
            break;
        case 2:
            printf(" SCH:   ");
            break;
        case 3:
            printf(" EX:    ");
            break;
        default:
            printf(" ------ ");
        }
    }
    printf("\n");

    // Print row for each lane in pipeline width
    unsigned int ex_i = 0;
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        if (p->FE_latch[i].valid)
        {
            printf(" %6lu ",
                   (unsigned long)p->FE_latch[i].inst.inst_num);
        }
        else
        {
            printf(" ------ ");
        }
        if (p->ID_latch[i].valid)
        {
            printf(" %6lu ",
                   (unsigned long)p->ID_latch[i].inst.inst_num);
        }
        else
        {
            printf(" ------ ");
        }
        if (p->SC_latch[i].valid)
        {
            printf(" %6lu ",
                   (unsigned long)p->SC_latch[i].inst.inst_num);
        }
        else
        {
            printf(" ------ ");
        }
        bool flag = false;
        for (; ex_i < MAX_WRITEBACKS; ex_i++)
        {
            if (p->EX_latch[ex_i].valid)
            {
                printf(" %6lu ",
                       (unsigned long)p->EX_latch[ex_i].inst.inst_num);
                ex_i++;
                flag = true;
                break;
            }
        }
        if (!flag) {
            printf(" ------ ");
        }
        printf("\n");
    }
    printf("\n");

    rat_print_state(p->rat);
    exeq_print_state(p->exeq);
    rob_print_state(p->rob);
}

/**
 * Simulate one cycle of all stages of a pipeline.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle(Pipeline *p)
{
    p->stat_num_cycle++;

    #ifdef DEBUG
        printf("\n--------------------------------------------\n");
        printf("Cycle count: %lu, retired instructions: %lu\n\n",
           (unsigned long)p->stat_num_cycle,
           (unsigned long)p->stat_retired_inst);
    #endif
    
    // In our simulator, stages are processed in reverse order.
    pipe_cycle_commit(p);
    pipe_cycle_writeback(p);
    pipe_cycle_exe(p);
    pipe_cycle_schedule(p);
    pipe_cycle_issue(p);
    pipe_cycle_decode(p);
    pipe_cycle_fetch(p);

    // Compile with "make debug" to have this show!
    #ifdef DEBUG
        pipe_print_state(p);
    #endif
}

/**
 * Simulate one cycle of the fetch stage of a pipeline.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_fetch(Pipeline *p)
{
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        if (!p->FE_latch[i].stall && !p->FE_latch[i].valid)
        {
            // No stall and latch empty, so fetch a new instruction.
            pipe_fetch_inst(p, &p->FE_latch[i]);
        }
    }
}

/**
 * Simulate one cycle of the instruction decode stage of a pipeline.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_decode(Pipeline *p)
{
    static uint64_t next_inst_num = 1;
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        if (!p->ID_latch[i].stall && !p->ID_latch[i].valid)
        {
            // No stall and latch empty, so decode the next instruction.
            // Loop to find the next in-order instruction.
            for (unsigned int j = 0; j < PIPE_WIDTH; j++)
            {
                if (p->FE_latch[j].valid &&
                    p->FE_latch[j].inst.inst_num == next_inst_num)
                {
                    p->ID_latch[i] = p->FE_latch[j];
                    p->FE_latch[j].valid = false;
                    next_inst_num++;
                    break;
                }
            }
        }
    }
}

/**
 * Simulate one cycle of the execute stage of a pipeline. This handles
 * instructions that take multiple cycles to execute.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_exe(Pipeline *p)
{
    // If all operations are single-cycle, copy SC latches to EX latches.
    if (LOAD_EXE_CYCLES == 1)
    {
        for (unsigned int i = 0; i < PIPE_WIDTH; i++)
        {
            if (p->SC_latch[i].valid)
            {
                p->EX_latch[i] = p->SC_latch[i];
                p->SC_latch[i].valid = false;
            }
        }
        return;
    }

    // Otherwise, we need to handle multi-cycle instructions with EXEQ.
    // All valid entries from the SC latches are inserted into the EXEQ.
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        if (p->SC_latch[i].valid)
        {
            if (!exeq_insert(p->exeq, p->SC_latch[i].inst))
            {
                fprintf(stderr, "Error: EXEQ full\n");
                p->halt = true;
                return;
            }

            p->SC_latch[i].valid = false;
        }
    }

    // Cycle the EXEQ to reduce wait time for each instruction by 1 cycle.
    exeq_cycle(p->exeq);

    // Transfer all finished entries from the EXEQ to the EX latch.
    for (unsigned int i = 0; i < MAX_WRITEBACKS && exeq_check_done(p->exeq); i++)
    {
        p->EX_latch[i].valid = true;
        p->EX_latch[i].stall = false;
        p->EX_latch[i].inst = exeq_remove(p->exeq);
    }
}

/**
 * Simulate one cycle of the issue stage of a pipeline: insert decoded
 * instructions into the ROB and perform register renaming.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_issue(Pipeline *p)
{
    // Perform a bubble sort on the instructions in the ID latch array.
    // This sorts the instructions by their instruction number in ascending order to ensure that instructions are processed in the correct order
    for (unsigned int i = 0; i < PIPE_WIDTH - 1; i++) 
    {
        for (unsigned int j = 0; j < PIPE_WIDTH - i - 1; j++) 
        {
            if ((p->ID_latch[j].inst.inst_num > p->ID_latch[j + 1].inst.inst_num)) 
            {
                PipelineLatchStruct temp = p->ID_latch[j];
                p->ID_latch[j] = p->ID_latch[j + 1];
                p->ID_latch[j + 1] = temp;
            }
        }
    }

    // Flag to indicate whether the previous cycle had a stall in the ID stage.
    bool prev_ID_stall = false;

    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        p->ID_latch[i].stall = prev_ID_stall;
        if (p->ID_latch[i].stall == true) 
            continue;

        // For each valid instruction
        if (p->ID_latch[i].valid == true && p->ID_latch[i].stall == false)
        {
            if (rob_check_space(p->rob)) 
            {
                // Checks if there is space in rob, and inserts if there is space
                int idx = rob_insert(p->rob, p->ID_latch[i].inst);
                
                if (idx != -1) {
                    // Setting the entry invalid if the inst is added into the ROB
                    p->ID_latch[i].valid = false;

                    // Checking if src1 is ready, and labelling the tag accordingly
                    if (p->rob->entries[idx].inst.src1_reg != -1) 
                    {
                        p->rob->entries[idx].inst.src1_tag = rat_get_remap(p->rat, p->rob->entries[idx].inst.src1_reg);
                        if (p->rob->entries[idx].inst.src1_tag == -1 || rob_check_ready(p->rob, p->rob->entries[idx].inst.src1_tag)) 
                        {
                            p->rob->entries[idx].inst.src1_ready = true;
                        }
                        else 
                        {
                            p->rob->entries[idx].inst.src1_ready = false;
                        }
                    }

                    // Checking if src2 is ready, and labelling the tag accordingly
                    if (p->rob->entries[idx].inst.src2_reg != -1) 
                    {
                        p->rob->entries[idx].inst.src2_tag = rat_get_remap(p->rat, p->rob->entries[idx].inst.src2_reg);
                        if (p->rob->entries[idx].inst.src2_tag == -1 || rob_check_ready(p->rob, p->rob->entries[idx].inst.src2_tag)) 
                        {
                            p->rob->entries[idx].inst.src2_ready = true;
                        }
                        else 
                        {
                            p->rob->entries[idx].inst.src2_ready = false;
                        }
                    }

                    // Register renaming
                    p->rob->entries[idx].inst.dr_tag = idx; 
                    // If this instruction writes to a register, update the RAT accordingly.
                    if (p->rob->entries[idx].inst.dest_reg != -1) 
                    {
                        rat_set_remap(p->rat, p->rob->entries[idx].inst.dest_reg, p->rob->entries[idx].inst.dr_tag);
                    }       
                }
            }
            else 
            {
                p->ID_latch[i].stall = true;
            }
            // to avoid further loops until stall is resolved
            prev_ID_stall = p->ID_latch[i].stall;
        }
    }    
}

/**
 * Simulate one cycle of the scheduling stage of a pipeline: schedule
 * instructions to execute if they are ready.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_schedule(Pipeline *p)
{
    // In-order scheduling
    if (SCHED_POLICY == SCHED_IN_ORDER)
    {        
        for (unsigned int i = 0; i < PIPE_WIDTH; i++) 
        {
            int j = p->rob->head_ptr;
            do 
            {
                // Find the oldest valid entry in the ROB that is not already executing
                if (p->rob->entries[j].valid == true && p->rob->entries[j].exec == false) 
                {
                    // Check if it is stalled
                    bool src1_ready = (p->rob->entries[j].inst.src1_reg == -1 || p->rob->entries[j].inst.src1_ready);
                    bool src2_ready = (p->rob->entries[j].inst.src2_reg == -1 || p->rob->entries[j].inst.src2_ready);
                    if(src1_ready == false || src2_ready == false) 
                    {
                        // Stop scheduling instructions
                        p->SC_latch[i].valid = false;
                    }
                    else 
                    {
                        // Send it to the next latch
                        rob_mark_exec(p->rob, p->rob->entries[j].inst);
                        p->SC_latch[i].inst = p->rob->entries[j].inst;
                        p->SC_latch[i].valid = true;
                    }
                    // Go to next lane once oldest entry is found
                    break;
                }
                j = (j + 1) % NUM_ROB_ENTRIES;
            } while(j != p->rob->tail_ptr);
        }
    }

    // Out-of-order scheduling
    if (SCHED_POLICY == SCHED_OUT_OF_ORDER)
    {
        for (unsigned int i = 0; i < PIPE_WIDTH; i++) 
        {
            int j = p->rob->head_ptr;
            do 
            {
                // Find the oldest valid entry in the ROB that has both source operands ready but is not already executing
                if (p->rob->entries[j].valid == true && p->rob->entries[j].exec == false) 
                {
                    // Check if it is stalled
                    bool src1_ready = (p->rob->entries[j].inst.src1_reg == -1 || p->rob->entries[j].inst.src1_ready);
                    bool src2_ready = (p->rob->entries[j].inst.src2_reg == -1 || p->rob->entries[j].inst.src2_ready);
                    if(src1_ready == false || src2_ready == false) 
                    {
                        // Stop scheduling instructions
                        p->SC_latch[i].valid = false;
                    }
                    else 
                    {
                        // Send it to the next latch
                        rob_mark_exec(p->rob, p->rob->entries[j].inst);
                        p->SC_latch[i].inst = p->rob->entries[j].inst;
                        p->SC_latch[i].valid = true;
                        // Go to next lane once oldest entry that is ready is found
                        break;
                    }
                }
                j = (j + 1) % NUM_ROB_ENTRIES;
            } while(j != p->rob->tail_ptr);
        }
    }
}

/**
 * Simulate one cycle of the writeback stage of a pipeline: update the ROB
 * with information from instructions that have finished executing.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_writeback(Pipeline *p)
{
    for (unsigned int i = 0; i < MAX_WRITEBACKS; i++) 
    {
        // For every valid instruction
        if (p->EX_latch[i].stall == false && p->EX_latch[i].valid == true) 
        {
            // Broadcast the result to all ROB entries
            rob_wakeup(p->rob, p->EX_latch[i].inst.dr_tag);
            // Mark the instruction ready to commit
            rob_mark_ready(p->rob, p->EX_latch[i].inst);
            p->EX_latch[i].valid = false;
        }
    }
}

/**
 * Simulate one cycle of the commit stage of a pipeline: commit instructions
 * in the ROB that are ready to commit.
 * 
 * @param p the pipeline to simulate
 */
void pipe_cycle_commit(Pipeline *p)
{
    for (unsigned int i = 0; i < PIPE_WIDTH; i++)
    {
        // Check if the instruction at the head of the ROB is ready to commit
        if (rob_check_head(p->rob))
        {
            // Remove head from the rob
            InstInfo headEntry = rob_remove_head(p->rob);
            // Commit that instruction
            pipe_commit_inst(p, headEntry);
            // Update rat
            int idx = rat_get_remap(p->rat, headEntry.dest_reg);
            if (idx == headEntry.dr_tag) 
            {
                rat_reset_entry(p->rat, headEntry.dest_reg);
            }
            if(rob_check_space(p->rob))
            {
                p->ID_latch[i].stall = false;
            }
            else 
            {
                p->ID_latch[i].stall = true;
            }
        }
    }
}
