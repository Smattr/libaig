#include <aig/aig.h>
#include "aig_t.h"
#include <assert.h>
#include <stdint.h>

uint64_t aig_max_index(const aig_t *aig) {
  assert(aig != NULL);
  return aig->max_index;
}

uint64_t aig_input_count(const aig_t *aig) {
  assert(aig != NULL);
  return aig->input_count;
}

uint64_t aig_latch_count(const aig_t *aig) {
  assert(aig != NULL);
  return aig->latch_count;
}

uint64_t aig_output_count(const aig_t *aig) {
  assert(aig != NULL);
  return aig->output_count;
}

uint64_t aig_and_count(const aig_t *aig) {
  assert(aig != NULL);
  return aig->and_count;
}
