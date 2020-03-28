#pragma once

#include <aig/aig.h>
#include "aig_t.h"
#include <stdint.h>

struct aig_node_iter {

  /// AIG we are iterating over
  aig_t *aig;

  /// index we are currently at
  uint64_t index;
};
