#include <aig/aig.h>
#include "aig_t.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

int aig_get_input(aig_t *aig, uint64_t index, struct aig_node *result) {

  if (aig == NULL)
    return EINVAL;

  if (result == NULL)
    return EINVAL;

  // does the AIG have at least this many inputs?
  if (index >= aig->input_count)
    return ERANGE;

  memset(result, 0, sizeof(*result));
  result->type = AIG_INPUT;
  result->index = index;

  // TODO: retrieve name from the symbol table

  return 0;
}

int aig_get_output(aig_t *aig, uint64_t index, struct aig_node *result) {

  if (aig == NULL)
    return EINVAL;

  if (result == NULL)
    return EINVAL;

  // does the AIG have at least this many outputs?
  if (index >= aig->output_count)
    return ERANGE;

  memset(result, 0, sizeof(*result));
  result->type = AIG_OUTPUT;
  result->index = index;

  // TODO: retrieve name from the symbol table

  return 0;
}
