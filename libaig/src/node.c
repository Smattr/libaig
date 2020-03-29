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
  if (index >= aig->input_count)
    return ERANGE;

  memset(result, 0, sizeof(*result));
  result->type = AIG_INPUT;
  result->variable_index = index + 1;

  // TODO: retrieve name from the symbol table

  return 0;
}

int aig_get_latch(aig_t *aig, uint64_t index, struct aig_node *result) {

  if (aig == NULL)
    return EINVAL;

  if (result == NULL)
    return EINVAL;

  // is this a valid latch in this AIG?
  if (index >= aig->latch_count)
    return ERANGE;

  // ensure we have this latch’s data available
  int rc = parse_latches(aig, index);
  if (rc)
    return rc;

  // retrieve the latch’s next state
  uint64_t next;
  if ((rc = bb_get(&aig->latches, index, bb_limit(aig), &next)))
    return rc;

  memset(result, 0, sizeof(*result));
  result->type = AIG_LATCH;
  result->variable_index = index + 1 + aig->input_count;
  result->next = next / 2;
  result->next_negated = next % 2;

  // TODO: retrieve name from the symbol table

  return 0;
}

int aig_get_output(aig_t *aig, uint64_t index, struct aig_node *result) {

  if (aig == NULL)
    return EINVAL;

  if (result == NULL)
    return EINVAL;

  // is this a valid output in this AIG?
  if (index >= aig->output_count)
    return ERANGE;

  // ensure we have this output’s data available
  int rc = parse_latches(aig, index);
  if (rc)
    return rc;

  // retrieve the output’s next state
  uint64_t o;
  if ((rc = bb_get(&aig->outputs, index, bb_limit(aig), &o)))
    return rc;

  memset(result, 0, sizeof(*result));
  result->type = AIG_OUTPUT;
  result->output = o / 2;
  result->output_negated = o % 2;

  // TODO: retrieve name from the symbol table

  return 0;
}
