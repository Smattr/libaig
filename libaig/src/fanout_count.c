#include <aig/aig.h>
#include "aig_t.h"
#include <errno.h>
#include <stddef.h>

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
