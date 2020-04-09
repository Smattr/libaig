#include <aig/aig.h>
#include "aig_t.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include "node_iter.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static bool is_fanout(const struct aig_node *n, uint64_t predecessor) {

  assert(n != NULL);

  switch (n->type) {

    case AIG_LATCH:
      if (n->latch.next == predecessor)
        return true;
      break;

    case AIG_AND_GATE:
      if (n->and_gate.rhs[0] == predecessor)
        return true;
      if (n->and_gate.rhs[1] == predecessor)
        return true;
      break;

    default:
      // do nothing
      break;

  }

  return false;
}

static uint64_t variable_index(const struct aig_node *node) {
  assert(node != NULL);
  switch (node->type) {
    case AIG_CONSTANT: return 0;
    case AIG_INPUT:    return node->input.variable_index;
    case AIG_LATCH:    return node->latch.current;
    case AIG_OUTPUT:   return node->output.variable_index;
    case AIG_AND_GATE: return node->and_gate.lhs;
  }
  __builtin_unreachable();
}

static int advance_to_next(aig_node_iter_t *it) {

  assert(it != NULL);
  assert(it->state != NULL);

  aig_t *aig = it->aig;
  int rc = 0;
  uint64_t predecessor = *(uint64_t*)it->state;

  while (it->index < aig->latch_count + aig->and_count) {

    // extract the current node
    struct aig_node n;
    if (it->index < aig->latch_count) {
      rc = aig_get_latch(aig, it->index, &n);
    } else {
      uint64_t index = it->index - aig->latch_count;
      rc = aig_get_and(aig, index, &n);
    }

    if (rc)
      return rc;

    if (is_fanout(&n, predecessor))
      break;

    ++it->index;
  }

  return 0;
}

static int next(aig_node_iter_t *it, struct aig_node *item) {

  assert(it != NULL);
  assert(item != NULL);
  assert(aig_iter_has_next(it));

  if (it->aig == NULL)
    return EINVAL;

  aig_t *aig = it->aig;
  int rc = 0;

  // extract the current node
  struct aig_node n;
  if (it->index < aig->latch_count) {
    rc = aig_get_latch(aig, it->index, &n);
  } else {
    uint64_t index = it->index - aig->latch_count;
    rc = aig_get_and(aig, index, &n);
  }

  if (rc)
    return rc;

  // move to the next valid node
  ++it->index;
  if ((rc = advance_to_next(it)))
    return rc;

  *item = n;
  return rc;
}

static bool has_next(const aig_node_iter_t *it) {

  assert(it != NULL);

  if (it->aig == NULL)
    return false;

  const aig_t *aig = it->aig;

  // if the current index is out of range, we are exhausted
  if (it->index >= aig->latch_count + aig->and_count)
    return false;

  // otherwise, there is more to consume
  return true;
}

static void fanout_free(aig_node_iter_t *it) {
  // clean up the predecessor index we saved
  free(it->state);
  it->state = NULL;
}

int aig_iter_fanout(aig_t *aig, const struct aig_node *node,
  aig_node_iter_t **it) {

  if (aig == NULL)
    return EINVAL;

  if (node == NULL)
    return EINVAL;

  if (it == NULL)
    return EINVAL;

  // create a new iterator;
  aig_node_iter_t *i = NULL;
  int rc = aig_iter(aig, &i);
  if (rc)
    return rc;

  // override the next-finding mechanism with our own
  i->has_next = has_next;
  i->next = next;
  i->free = fanout_free;

  // save the predecessor index within the iterator
  uint64_t index = variable_index(node);
  i->state = calloc(1, sizeof(uint64_t));
  if (i->state == NULL) {
    aig_iter_free(&i);
    return ENOMEM;
  }
  *(uint64_t*)i->state = index;

  // advance until we hit a valid fanout or exhaust the nodes
  if ((rc = advance_to_next(i))) {
    aig_iter_free(&i);
    return rc;
  }

  *it = i;

  return 0;
}
