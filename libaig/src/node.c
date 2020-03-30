#include <aig/aig.h>
#include "aig_t.h"
#include "bitbuffer.h"
#include <errno.h>
#include "infer.h"
#include "parse.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

int aig_get_input_no_symbol(aig_t *aig, uint64_t index,
    struct aig_node *result) {

  if (aig == NULL)
    return EINVAL;

  if (result == NULL)
    return EINVAL;

  // is this a valid input in this AIG?
  if (index >= aig->input_count)
    return ERANGE;

  // ensure we have this input’s data available
  int rc = parse_inputs(aig, index);
  if (rc)
    return rc;

  // retrieve the encoded index of the input
  uint64_t input = get_input(aig, index);

  memset(result, 0, sizeof(*result));
  result->type = AIG_INPUT;
  result->input.variable_index = input / 2;

  // add the symbol name if we have it
  uint64_t symtab_index = index;
  if (aig->symtab != NULL)
    result->input.name = aig->symtab[symtab_index];

  return 0;
}

int aig_get_input(aig_t *aig, uint64_t index, struct aig_node *result) {

  if (aig == NULL)
    return EINVAL;

  if (result == NULL)
    return EINVAL;

  // is this a valid input in this AIG?
  if (index >= aig->input_count)
    return ERANGE;

  // ensure the input’s symbol (if it exists) is in memory before looking it up
  uint64_t symtab_index = index;
  int rc = parse_symtab(aig, symtab_index);
  if (rc)
    return rc;

  return aig_get_input_no_symbol(aig, index, result);
}

int aig_get_latch_no_symbol(aig_t *aig, uint64_t index,
    struct aig_node *result) {

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
  result->latch.current = index + 1 + aig->input_count;
  result->latch.next = next / 2;
  result->latch.next_negated = next % 2;

  // add the symbol name if we have it
  uint64_t symtab_index = index + aig->input_count;
  if (aig->symtab != NULL)
    result->latch.name = aig->symtab[symtab_index];

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

  // ensure the latch’s symbol (if it exists) is in memory before looking it up
  uint64_t symtab_index = index + aig->input_count;
  int rc = parse_symtab(aig, symtab_index);
  if (rc)
    return rc;

  return aig_get_latch_no_symbol(aig, index, result);
}

int aig_get_output_no_symbol(aig_t *aig, uint64_t index,
    struct aig_node *result) {

  if (aig == NULL)
    return EINVAL;

  if (result == NULL)
    return EINVAL;

  // is this a valid output in this AIG?
  if (index >= aig->output_count)
    return ERANGE;

  // ensure we have this output’s data available
  int rc = parse_outputs(aig, index);
  if (rc)
    return rc;

  // retrieve the output’s next state
  uint64_t o;
  if ((rc = bb_get(&aig->outputs, index, bb_limit(aig), &o)))
    return rc;

  memset(result, 0, sizeof(*result));
  result->type = AIG_OUTPUT;
  result->output.variable_index = o / 2;
  result->output.negated = o % 2;

  // add the symbol name if we have it
  uint64_t symtab_index = index + aig->input_count + aig->latch_count;
  if (aig->symtab != NULL)
    result->output.name = aig->symtab[symtab_index];

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

  // ensure the output’s symbol (if it exists) is in memory before looking it up
  uint64_t symtab_index = index + aig->input_count + aig->latch_count;
  int rc = parse_symtab(aig, symtab_index);
  if (rc)
    return rc;

  return aig_get_output_no_symbol(aig, index, result);
}

int aig_get_and(aig_t *aig, uint64_t index, struct aig_node *result) {

  if (aig == NULL)
    return EINVAL;

  if (result == NULL)
    return EINVAL;

  // is this a valid AND gate in this AIG?
  if (index >= aig->and_count)
    return ERANGE;

  // ensure we have this AND gate’s data available
  int rc = parse_ands(aig, index);
  if (rc)
    return rc;

  // retrieve the AND gate’s LHS
  uint64_t lhs;
  if (aig->binary) {
    // for a binary AIG, we can infer the LHS because the AND gates are ordered
    // and consecutive
    lhs = get_inferred_and_lhs(aig, index);
  } else {
    // for an ASCII AIG, the LHS is either inferable, in which case we have no
    // LHS data, or it is stored in a bit buffer
    if (bb_is_empty(&aig->and_lhs)) { // inferable
      lhs = get_inferred_and_lhs(aig, index);
    } else { // stored
      if ((rc = bb_get(&aig->and_lhs, index, bb_limit(aig), &lhs)))
        return rc;
    }
  }

  // retrieve the AND gate’s RHS
  uint64_t rhs0, rhs1;
  if ((rc = bb_get(&aig->and_rhs, index * 2, bb_limit(aig), &rhs0)))
    return rc;
  if ((rc = bb_get(&aig->and_rhs, index * 2 + 1, bb_limit(aig), &rhs1)))
    return rc;

  memset(result, 0, sizeof(*result));
  result->type = AIG_AND_GATE;
  result->and_gate.lhs = lhs / 2;
  result->and_gate.rhs[0] = rhs0 / 2;
  result->and_gate.rhs[1] = rhs1 / 2;
  result->and_gate.negated[0] = rhs0 % 2;
  result->and_gate.negated[1] = rhs1 % 2;

  return 0;
}

int aig_get_node(aig_t *aig, uint64_t variable_index, struct aig_node *result) {

  if (aig == NULL)
    return EINVAL;

  if (result == NULL)
    return EINVAL;

  uint64_t i = variable_index;

  // index 0 is the constant FALSE
  if (i == 0) {
    memset(result, 0, sizeof(*result));
    result->type = AIG_CONSTANT;
    return 0;
  }
  i--;

  if (i < aig->input_count)
    return aig_get_input(aig, i, result);
  i -= aig->input_count;

  if (i < aig->latch_count)
    return aig_get_latch(aig, i, result);
  i -= aig->latch_count;

  if (i < aig->and_count)
    return aig_get_and(aig, i, result);

  return ERANGE;
}
