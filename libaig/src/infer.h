#pragma once

#include <aig/aig.h>
#include "aig_t.h"
#include <stdint.h>

/** get the LHS that would be inferred from an AND gate
 *
 * \param aig Structure to examine
 * \param index Index of the AND gate
 * \returns The inferred encoded LHS of this AND gate
 */
__attribute__((visibility("internal")))
uint64_t get_inferred_and_lhs(const aig_t *aig, uint64_t index);

/** get the LHS of an already-parsed AND gate
 *
 * \param aig Structure to examine
 * \param index Index of the AND gate to lookup
 * \returns The encoded LHS of this AND gate
 */
__attribute__((visibility("internal")))
uint64_t get_and_lhs(const aig_t *aig, uint64_t index);
