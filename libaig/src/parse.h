#pragma once

#include <aig/aig.h>
#include "aig_t.h"
#include <stdint.h>

/** parse the header of an AIG file
 *
 * It is assumed that aig->source is an open file handle pointing to the start
 * of the file. This function will read from there, assuming the header will be
 * the first thing it reads.
 *
 * \param aig Data structure to read from and into
 * \returns 0 on success or an errno on failure
 */
__attribute__((visibility("internal")))
int parse_header(aig_t *aig);

/** parse the input section of an AIG file
 *
 * The upto index does not have to be a valid input index within the given AIG.
 * So you can pass UINT64_MAX to parse the entire input section.
 *
 * \param aig Data structure to read from
 * \param upto Input index after which to stop parsing
 * \returns 0 on success or an errno on failure
 */
__attribute__((visibility("internal")))
int parse_inputs(aig_t *aig, uint64_t upto);

/** parse the latches section of an AIG file
 *
 * The upto index does not have to be a valid latch index within the given AIG.
 * So you can pass UINT64_MAX to parse the entire latch section.
 *
 * \param aig Data structure to read from
 * \param upto Latch index after which to stop parsing
 * \returns 0 on success or an errno on failure
 */
__attribute__((visibility("internal")))
int parse_latches(aig_t *aig, uint64_t upto);
