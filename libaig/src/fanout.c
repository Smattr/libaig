#include <aig/aig.h>
#include "aig_t.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include "node_iter.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool is_fanout(aig_t *aig, uint64_t index, void *state) {

  assert(aig != NULL);
  assert(state != NULL);

  uint64_t predecessor = (uint64_t)(uintptr_t)state;

  struct aig_node n;
  if (aig_get_node(aig, index, &n)) {
    // if we failed to retrieve this node, not much we can do
    return false;
  }

  switch (n.type) {

    case AIG_LATCH:
      if (n.latch.next == predecessor)
        return true;
      break;

    case AIG_AND_GATE:
      if (n.and_gate.rhs[0] == predecessor)
        return true;
      if (n.and_gate.rhs[1] == predecessor)
        return true;
      break;

    default:
      // do nothing
      break;

  }

  return false;
}

static uint64_t index(const struct aig_node *node) {
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

int aig_iter_fanout(aig_t *aig, const struct aig_node *node,
  aig_node_iter_t **it) {

  if (aig == NULL)
    return EINVAL;

  if (node == NULL)
    return EINVAL;

  if (it == NULL)
    return EINVAL;

  // if we are on a 32-bit platform, it is possible the node’s variable index
  // will not fit in the predicate state member
  if (sizeof(uint64_t) > sizeof((*it)->predicate_state) &&
      index(node) > (uint64_t)UINTPTR_MAX)
    return ERANGE;

  // create a new iterator;
  aig_node_iter_t *i = NULL;
  int rc = aig_iter(aig, &i);
  if (rc)
    return rc;

  void *predecessor = (void*)(uintptr_t)index(node);

  // advance the iterator until it is exhausted or we hit a valid node
  while (i->index < aig->input_count + aig->latch_count + aig->and_count
      + aig->output_count) {

    if (is_fanout(aig, i->index, predecessor))
      break;

    ++i->index;
  }

  i->predicate = is_fanout;
  i->predicate_state = predecessor;
  *it = i;

  return 0;
}

int aig_fanout_count(aig_t *aig, const struct aig_node *node, size_t *count) {

  if (aig == NULL)
    return EINVAL;

  if (node == NULL)
    return EINVAL;

  if (count == NULL)
    return EINVAL;

  int rc = 0;

  // create a fanout iterator
  aig_node_iter_t *it = NULL;
  if ((rc = aig_iter_fanout(aig, node, &it)))
    return rc;

  // iterate through all nodes, counting the total
  size_t c = 0;
  while (aig_iter_has_next(it)) {
    ++c;
    struct aig_node ignored;
    if ((rc = aig_iter_next(it, &ignored)))
      return rc;
  }

  // clean up the iterator
  aig_iter_free(&it);

  *count = c;
  return 0;
}
