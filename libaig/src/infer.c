#include <aig/aig.h>
#include "aig_t.h"
#include <assert.h>
#include "bitbuffer.h"
#include "infer.h"
#include <stddef.h>
#include <stdint.h>

uint64_t get_inferred_input(const aig_t *aig, uint64_t index) {
  assert(aig != NULL);
  (void)aig;
  return (1 + index) * 2;
}

uint64_t get_input(const aig_t *aig, uint64_t index) {
  assert(aig != NULL);

  // if the AIG has no inputs data, the input was deemed inferable
  if (bb_is_empty(&aig->inputs))
    return get_inferred_input(aig, index);

  // otherwise it should be in the input array;
  uint64_t input;
  int rc __attribute__((unused))
    = bb_get(&aig->inputs, index, bb_limit(aig), &input);
  assert(rc == 0);

  return input;
}

uint64_t get_inferred_and_lhs(const aig_t *aig, uint64_t index) {
  assert(aig != NULL);
  return (1 + aig->input_count + aig->latch_count + index) * 2;
}

uint64_t get_and_lhs(const aig_t *aig, uint64_t index) {
  assert(aig != NULL);

  // if the AIG has no LHS data, the LHS was deemed inferable
  if (bb_is_empty(&aig->and_lhs))
    return get_inferred_and_lhs(aig, index);

  // otherwise it should be in the LHS array
  uint64_t lhs;
  int rc __attribute__((unused))
    = bb_get(&aig->and_lhs, index, bb_limit(aig), &lhs);
  assert(rc == 0);

  return lhs;
}
