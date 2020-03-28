#include <aig/aig.h>
#include "aig_t.h"
#include "bitbuffer.h"
#include <errno.h>
#include "parse.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

int aig_get_input(aig_t *aig, uint64_t index, struct aig_node *result) {

  if (aig == NULL)
    return EINVAL;

  if (result == NULL)
    return EINVAL;

  // is this a valid input in this AIG?
  if (index == 0)
    return ERANGE;
  if (index > aig->input_count)
    return ERANGE;

  memset(result, 0, sizeof(*result));
  result->type = AIG_INPUT;
  result->index = index;

  // TODO: retrieve name from the symbol table

  return 0;
}

int aig_get_latch(aig_t *aig, uint64_t index, struct aig_node *result) {

  if (aig == NULL)
    return EINVAL;

  if (result == NULL)
    return EINVAL;

  // is this a valid latch in this AIG?
  if (index <= aig->input_count)
    return ERANGE;
  if (index > aig->input_count + aig->latch_count)
    return ERANGE;

  // ensure we have this latch’s data available
  int rc = parse_latches(aig, index + 1);
  if (rc)
    return rc;

  // retrieve the latch’s next state
  uint64_t next;
  uint64_t buffer_index = index - aig->input_count - 1;
  if ((rc = bb_get(&aig->latches, buffer_index, aig->max_index * 2 + 1, &next)))
    return rc;

  memset(result, 0, sizeof(*result));
  result->type = AIG_LATCH;
  result->index = index;
  result->current = index;
  result->next = next;

  // TODO: retrieve name from the symbol table

  return 0;
}

int aig_get_output(aig_t *aig, uint64_t index, struct aig_node *result) {

  if (aig == NULL)
    return EINVAL;

  if (result == NULL)
    return EINVAL;

  // is this a valid output in this AIG?
  if (index <= aig->input_count + aig->latch_count)
    return ERANGE;
  if (index > aig->input_count + aig->latch_count + aig->output_count)
    return ERANGE;

  memset(result, 0, sizeof(*result));
  result->type = AIG_OUTPUT;
  result->index = index;

  // TODO: retrieve name from the symbol table

  return 0;
}
