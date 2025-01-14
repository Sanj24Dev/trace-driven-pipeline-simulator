// rat.h
// Declares the struct for the register alias table.

#ifndef _RAT_H_
#define _RAT_H_

#include <inttypes.h>

/** The number of registers in the architecture as defined by the ISA. */
#define MAX_ARF_REGS 32

/** A single register alias in the RAT. */
typedef struct RATEntryStruct
{
    /**
     * Is this alias valid?
     * 
     * If so, prf_id refers to a valid entry in the ROB which will contain the
     * latest value of this register.
     * If not, the register's latest value is already committed in the ARF.
     */
    bool valid;

    /**
     * The Physical Register File ID of the ROB entry whose output this register is aliased to.
     */
    uint64_t prf_id;
} RATEntry;

/**
 * The register alias table.
 * 
 * The RAT consists of an array of entries, one for each architectural
 * register. This array is indexed by the architectural register ID.
 */
typedef struct RAT
{
    /**
     * An array of register aliases, indexed by the architectural register ID.
     */
    RATEntry entries[MAX_ARF_REGS];
} RAT;

/**
 * Allocate and initialize a new RAT
 * 
 * @return a pointer to a newly allocated RAT
 */
RAT *rat_init();

/**
 * Print out the state of the RAT for debugging purposes
 * 
 * @param rat the RAT
 */
void rat_print_state(RAT *rat);

/**
 * Get the PRF ID (i.e., ID of ROB entry) of the latest value of a register.
 * 
 * If the register is not currently aliased (i.e., its latest value is already
 * committed and thus resides in the ARF), return -1
 * 
 * @param rat the RAT
 * @param arf_id the ID of the architectural register to get the alias of
 * @return the ID of the ROB entry whose output this register is aliased to, or
 *         -1 if the register is not aliased
 */
int rat_get_remap(RAT *rat, int arf_id);

/**
 * Set the PRF ID (i.e., ID of ROB entry) that a register should be aliased to
 * 
 * @param rat the RAT
 * @param arf_id the ID of the architectural register to set the alias of
 * @param prf_id the ID of the ROB entry whose output this register should be
 *               aliased to
 */
void rat_set_remap(RAT *rat, int arf_id, int prf_id);

/**
 * Reset the alias of a register
 * 
 * @param rat the RAT
 * @param arf_id the ID of the architectural register to reset the alias of
 */
void rat_reset_entry(RAT *rat, int arf_id);

#endif
