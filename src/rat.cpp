// rat.cpp
/////////////////////////////////////////////////////////////////////////////
// Contains the following implementation to simulate register alias table: //
// - int rat_get_remap(RAT *rat, int arf_id)                               //
// - void rat_set_remap(RAT *t, int arf_id, int prf_id)                    //
// - void rat_reset_entry(RAT *t, int arf_id)                              //
/////////////////////////////////////////////////////////////////////////////

#include "rat.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * Allocate and initialize a new RAT
 * 
 * @return a pointer to a newly allocated RAT
 */
RAT *rat_init()
{
    RAT *rat = (RAT *)calloc(1, sizeof(RAT));
    for (int i = 0; i < MAX_ARF_REGS; i++)
    {
        rat->entries[i].valid = false;
    }
    return rat;
}

/**
 * Print out the state of the RAT for debugging purposes
 * 
 * @param rat the RAT
 */
void rat_print_state(RAT *rat)
{
    int ii = 0;
    printf("Current RAT state:\n");
    printf("Entry  Valid\tprf_id\n");
    for (ii = 0; ii < MAX_ARF_REGS; ii++)
    {
        printf("%5d ::  %d \t", ii, rat->entries[ii].valid);
        printf("%5d \n", (int)rat->entries[ii].prf_id);
    }
    printf("\n");
}

/**
 * Get the PRF ID (i.e., ID of ROB entry) of the latest value of a register
 * 
 * If the register is not currently aliased (i.e., its latest value is already
 * committed and thus resides in the ARF), return -1
 * 
 * @param rat the RAT
 * @param arf_id the ID of the architectural register to get the alias of
 * @return the ID of the ROB entry whose output this register is aliased to, or
 *         -1 if the register is not aliased
 */
int rat_get_remap(RAT *rat, int arf_id)
{
    // Access the RAT entry
    if(rat->entries[arf_id].valid) 
    {
        // Return the PRF ID
        return rat->entries[arf_id].prf_id;
    }
    else 
    {
        return -1;
    }
}

/**
 * Set the PRF ID (i.e., ID of ROB entry) that a register should be aliased to.
 * 
 * @param rat the RAT
 * @param arf_id the ID of the architectural register to set the alias of
 * @param prf_id the ID of the ROB entry whose output this register should be
 *               aliased to
 */
void rat_set_remap(RAT *rat, int arf_id, int prf_id)
{
    // Access the RAT entry at the correct index.
    rat->entries[arf_id].valid = true;
    // Set the correct values on that entry.
    rat->entries[arf_id].prf_id = prf_id;
}

/**
 * Reset the alias of a register.
 * 
 * @param rat the RAT
 * @param arf_id the ID of the architectural register to reset the alias of
 */
void rat_reset_entry(RAT *rat, int arf_id)
{
    rat->entries[arf_id].valid = false;
}
