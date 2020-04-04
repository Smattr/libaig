#include <aig/aig.h>
#include "aig_t.h"
#include <assert.h>
#include <errno.h>
#include "node_iter.h"
#include <stdbool.h>
#include <stdlib.h>

int aig_iter(aig_t *aig, aig_node_iter_t **it) {

  if (aig == NULL)
    return EINVAL;

  if (it == NULL)
    return EINVAL;

  // allocate a new iterator
  aig_node_iter_t *i = calloc(1, sizeof(*i));
  if (i == NULL)
    return ENOMEM;

  // set it up to point at the start of this AIG
  i->aig = aig;
  i->index = 0;

  *it = i;
  return 0;
}

bool aig_iter_has_next(const aig_node_iter_t *it) {

  if (it == NULL)
    return false;

  if (it->aig == NULL)
    return false;

  const aig_t *aig = it->aig;

  // if the current index is out of range, we are exhausted
  if (it->index >= aig->input_count + aig->latch_count + aig->and_count
      + aig->output_count)
    return false;

  // otherwise, there is more to consume
  return true;
}

static void move_next(aig_node_iter_t *it) {

  assert(it != NULL);

  do {

    // move to the next index
    ++it->index;

    // loop while we have not exhausted the iterator and have not satisfied the
    // predicate
  } while (aig_iter_has_next(it) &&
    it->predicate != NULL &&
    !it->predicate(it->aig, it->index, it->predicate_state));
}

int aig_iter_next(aig_node_iter_t *it, struct aig_node *item) {

  if (it == NULL)
    return EINVAL;

  if (!aig_iter_has_next(it))
    return EINVAL;

  if (item == NULL)
    return EINVAL;

  assert(it->aig != NULL && "invalid iterator state");

  uint64_t index = it->index;

  // are we currently pointing at an input?
  if (index < it->aig->input_count) {
    int rc = aig_get_input(it->aig, index, item);
    move_next(it);
    return rc;
  }
  index -= it->aig->input_count;

  // are we currently pointing at a latch?
  if (index < it->aig->latch_count) {
    int rc = aig_get_latch(it->aig, index, item);
    move_next(it);
    return rc;
  }
  index -= it->aig->latch_count;

  // are we currently pointing at an output?
  if (index < it->aig->output_count) {
    int rc = aig_get_output(it->aig, index, item);
    move_next(it);
    return rc;
  }
  index -= it->aig->output_count;

  // if we have reached here, we must be up to an AND gate
  assert(index < it->aig->and_count && "incorrect aig_iter_next() logic");

  int rc = aig_get_and(it->aig, index, item);
  move_next(it);
  return rc;
}

void aig_iter_free(aig_node_iter_t **it) {

  if (it == NULL)
    return;

  if (*it == NULL)
    return;

  free(*it);
  *it = NULL;
}
