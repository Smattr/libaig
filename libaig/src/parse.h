#pragma once

#include <aig/aig.h>
#include "aig_t.h"

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
