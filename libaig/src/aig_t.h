#pragma once

#include <stdbool.h>
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

  /// was the source binary encoded instead of ASCII?
  bool binary;

  /// input file (or in-memory buffer) AIG was read from
  FILE *source;
};
