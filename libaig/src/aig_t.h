#pragma once

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

  /// was the source binary encoded instead of ASCII?
  uint8_t binary:1;

  /// are we using strict parsing mode?
  uint8_t strict:1;

  /// are we using eager loading mode?
  uint8_t eager:1;
};
