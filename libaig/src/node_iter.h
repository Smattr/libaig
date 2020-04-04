#pragma once

#include <aig/aig.h>
#include "aig_t.h"
#include <stdint.h>

struct aig_node_iter {

  /// AIG we are iterating over
  aig_t *aig;

  /// index we are currently at
  uint64_t index;

  /// optional filter to apply during iteration
  bool (*predicate)(aig_t *aig, uint64_t index, void *state);

  /// optional state for the filter
  void *predicate_state;
};
