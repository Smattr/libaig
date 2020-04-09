#pragma once

#include <aig/aig.h>
#include "aig_t.h"
#include <stdbool.h>
#include <stdint.h>

struct aig_node_iter {

  /// AIG we are iterating over
  aig_t *aig;

  /// index we are currently at
  uint64_t index;

  /** check if this iterator is not exhausted
   *
   * \param it The iterator to examine
   * \returns True if the iterator can yield more items
   */
  bool (*has_next)(const aig_node_iter_t *it);

  /** get the next item and advance the iterator
   *
   * \param it Iterator to operate on
   * \param item [out] The next node in the iteration on success
   * \returns 0 on success or an errno on failure
   */
  int (*next)(aig_node_iter_t *it, struct aig_node *item);

  /** clean up this iterator
   *
   * \param it Iterator whose resources should be deallocated
   */
  void (*free)(aig_node_iter_t *it);

  /// optional state for this iterator
  void *state;
};
