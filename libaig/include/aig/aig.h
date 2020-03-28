#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#ifdef __cpluscplus
extern "C" {
#endif

/// an opaque handle to an AIG
typedef struct aig aig_t;

/// options that can be specified when creating an AIG handle
struct aig_options {

  /// demand exact white space conformance when parsing
  bool strict;

  /// parse entire AIG file on load
  bool eager;
};

// AIG lifetime functions /////////////////////////////////////////////////////

/** allocate and initialise a new, blank AIG
 *
 * \param aig [out] Handle to the initialised data structure
 * \returns 0 on success or an errno on failure
 */
int aig_new(aig_t **aig, struct aig_options options);

/** allocate a new AIG and initialise it from the given file
 *
 * \param [out] Handle to the initialised data structure
 * \param filename Source file to read
 * \returns 0 on success or an errno on failure
 */
int aig_load(aig_t **aig, const char *filename, struct aig_options options);

/** allocate a new AIG and initialise it from the given file pointer
 *
 * \param [out] Handle to the initialised data structure
 * \param f Source file handle to read from
 * \returns 0 on success or an errno on failure
 */
int aig_loadf(aig_t **aig, FILE *f, struct aig_options options);

/** deallocate resources associated with an AIG
 *
 * \param aig [in,out] Handle to data structure to deallocate and set to NULL
 */
void aig_free(aig_t **aig);

////////////////////////////////////////////////////////////////////////////////

// header/summary access ///////////////////////////////////////////////////////

/** get the maximum variable index (M) of this AIG
 *
 * \param aig Handle to AIG data structure to read from
 * \returns maximum variable index
 */
uint64_t aig_max_index(const aig_t *aig);

/** get the number of inputs (I) to this AIG
 *
 * \param aig Handle to AIG data structure to read from
 * \returns number of inputs
 */
uint64_t aig_input_count(const aig_t *aig);

/** get the number of latches (L) in this AIG
 *
 * \param aig Handle to AIG data structure to read from
 * \returns number of latches
 */
uint64_t aig_latch_count(const aig_t *aig);

/** get the number of outputs (O) from this AIG
 *
 * \param aig Handle to AIG data structure to read from
 * \returns number of outputs
 */
uint64_t aig_output_count(const aig_t *aig);

/** get the number of AND gates (A) in this AIG
 *
 * \param aig Handle to AIG data structure to read from
 * \returns number of AND gates
 */
uint64_t aig_and_count(const aig_t *aig);

////////////////////////////////////////////////////////////////////////////////

#ifdef __cpluscplus
}
#endif
