#pragma once

#include <stdio.h>

#ifdef __cpluscplus
extern "C" {
#endif

/// an opaque handle to an AIG
typedef struct aig aig_t;

/** allocate and initialise a new, blank AIG
 *
 * \param aig [out] Handle to the initialised data structure
 * \returns 0 on success or an errno on failure
 */
int aig_new(aig_t **aig);

/** allocate a new AIG and initialise it from the given file
 *
 * \param [out] Handle to the initialised data structure
 * \param filename Source file to read
 * \returns 0 on success or an errno on failure
 */
int aig_load(aig_t **aig, const char *filename);

/** allocate a new AIG and initialise it from the given file pointer
 *
 * \param [out] Handle to the initialised data structure
 * \param f Source file handle to read from
 * \returns 0 on success or an errno on failure
 */
int aig_loadf(aig_t **aig, FILE *f);

/** deallocate resources associated with an AIG
 *
 * \param aig [in,out] Handle to data structure to deallocate and set to NULL
 */
void aig_free(aig_t **aig);

#ifdef __cpluscplus
}
#endif
