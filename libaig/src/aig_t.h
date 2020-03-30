#pragma once

#include <aig/aig.h>
#include <assert.h>
#include "bitbuffer.h"
#include <stdio.h>
#include <stdint.h>

struct __attribute__((visibility("internal"))) aig {

  /// maximum variable index
  uint64_t max_index;

  /// number of inputs
  uint64_t input_count;

  /// number of latches
  uint64_t latch_count;

  /// number of outputs
  uint64_t output_count;

  /// number of AND gates
  uint64_t and_count;

  /// input file (or in-memory buffer) AIG was read from
  FILE *source;

  /// next state values for each latch
  bitbuffer_t latches;

  /// output nodes
  bitbuffer_t outputs;

  /// LHS of AND gates
  bitbuffer_t and_lhs;

  /// RHSs of AND gates
  bitbuffer_t and_rhs;

  /// optional symbol table
  char **symtab;

  /// internal parsing state
  struct {
    enum state {
      IN_INPUTS,
      IN_LATCHES,
      IN_OUTPUTS,
      IN_ANDS,
      IN_SYMTAB,
      DONE,
    } state;
    uint64_t index;
  };

  /// was the source binary encoded instead of ASCII?
  uint8_t binary:1;

  /// are we using strict parsing mode?
  uint8_t strict:1;

  /// are we using eager loading mode?
  uint8_t eager:1;
};

/** get the limit value to use for bit buffers in an AIG struct
 *
 * \param aig Structure to examine
 * \returns Limit value for use with bb_append(), bb_get()
 */
static inline uint64_t bb_limit(const aig_t *aig) {
  assert(aig != NULL);

  // the bit buffers store elements that are representations of possibly-negated
  // variable indices as they appear in the AIGER file format
  return aig->max_index * 2 + 1;
}
