#include <aig/aig.h>
#include "aig_t.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

static size_t max(size_t a, size_t b) {
  if (a > b)
    return a;
  return b;
}

static int cache_level(aig_t *aig, size_t index, size_t level) {
  assert(aig != NULL);
  assert(index <= aig->max_index);

  // do we need to create the cache first?
  if (aig->levels == NULL) {
    aig->levels = calloc(aig->max_index, sizeof(aig->levels[0]));
    if (aig->levels == NULL)
      return ENOMEM;
  }

  aig->levels[index] = level;

  return 0;
}

int aig_node_level(aig_t *aig, const struct aig_node *node, size_t *level) {

  if (aig == NULL)
    return EINVAL;

  if (node == NULL)
    return EINVAL;

  if (level == NULL)
    return EINVAL;

  switch (node->type) {

    // constants and inputs are both base nodes
    case AIG_CONSTANT:
    case AIG_INPUT:
      *level = 0;
      return 0;

    case AIG_LATCH: {

      // does this latch have an invalid index (inconsistent AIG)?
      if (node->latch.current > aig->max_index)
        return ERANGE;

      // if we have the level of this node cached, retrieve from there
      if (aig->levels != NULL && aig->levels[node->latch.current] != 0) {
        *level = aig->levels[node->latch.current];
        return 0;
      }

      // find the node for its next state
      struct aig_node n;
      int rc = aig_get_node(aig, node->latch.next, &n);
      if (rc)
        return rc;

      // find the level of that node
      size_t l;
      if ((rc = aig_node_level(aig, &n, &l)))
        return rc;

      // then we are one deeper
      *level = l + 1;

      // cache this for future calls; ignore failure as it is tolerable
      (void)cache_level(aig, node->latch.current, *level);

      return 0;
    }

    case AIG_OUTPUT: {

      // does this output have an invalid index?
      if (node->output.variable_index > aig->max_index)
        return ERANGE;

      // if we have the level of this node cached, retrieve from there
      if (aig->levels != NULL && aig->levels[node->output.variable_index] != 0) {
        *level = aig->levels[node->output.variable_index];
        return 0;
      }

      // if the user gave us an output, look up the input, latch, or AND gate it
      // is actually wired up to
      struct aig_node n;
      int rc = aig_get_node(aig, node->output.variable_index, &n);
      if (rc)
        return rc;

      // then delegate to finding the level of that
      return aig_node_level(aig, &n, level);
    }

    case AIG_AND_GATE: {

      // does this AND gate have an invalid index?
      if (node->and_gate.lhs > aig->max_index)
        return ERANGE;

      // if we have the level of this node cached, retrieve from there
      if (aig->levels != NULL && aig->levels[node->input.variable_index] != 0) {
        *level = aig->levels[node->input.variable_index];
        return 0;
      }

      // find the node for its first RHS term
      struct aig_node rhs0;
      int rc = aig_get_node(aig, node->and_gate.rhs[0], &rhs0);
      if (rc)
        return rc;

      // find the level of this node
      size_t l_rhs0;
      if ((rc = aig_node_level(aig, &rhs0, &l_rhs0)))
        return rc;

      // now repeat the process for the second RHS term
      struct aig_node rhs1;
      if ((rc = aig_get_node(aig, node->and_gate.rhs[1], &rhs1)))
        return rc;

      size_t l_rhs1;
      if ((rc = aig_node_level(aig, &rhs1, &l_rhs1)))
        return rc;

      // our level is one more than the greatest of these
      *level = max(l_rhs0, l_rhs1) + 1;

      // cache this for future calls; ignore failure as it is tolerable
      (void)cache_level(aig, node->and_gate.lhs, *level);

      return 0;
    }
  }
}
