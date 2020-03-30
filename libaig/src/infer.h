#pragma once

#include <aig/aig.h>
#include "aig_t.h"
#include <stdint.h>

/** get the input that would be inferred from an input’s position
 *
 * \param aig Structure to examine
 * \param index Index of the input
 * \returns The inferred encoded input
 */
__attribute__((visibility("internal")))
uint64_t get_inferred_input(const aig_t *aig, uint64_t index);

/** get the encoded input of an already parsed input
 *
 * \param aig Structure to examine
 * \param index Index of the input to lookup
 * \returns The inferred encoded input
 */
__attribute__((visibility("internal")))
uint64_t get_input(const aig_t *aig, uint64_t index);

/** get the current value of that would be inferred from a latch’s position
 *
 * \param aig Structure to examine
 * \param index Index of the latch
 * \returns The inferred encoded latch
 */
__attribute__((visibility("internal")))
uint64_t get_inferred_latch_current(const aig_t *aig, uint64_t index);

/** get the current value from an already parsed latch
 *
 * \param aig Structure to examine
 * \param index Index of the latch to lookup
 * \returns The inferred encoded current value of the latch
 */
__attribute__((visibility("internal")))
uint64_t get_latch_current(const aig_t *aig, uint64_t index);

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
