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

// node access /////////////////////////////////////////////////////////////////

/// the type of an AIG node
enum aig_node_type {
  AIG_INPUT,
  AIG_LATCH,
  AIG_OUTPUT,
  AIG_AND_GATE,
};

/// a node in an AIG
struct aig_node {

  /// type of this node
  enum aig_node_type type;

  /// index in its containing AIG
  uint64_t index;

  /// optional name from the AIG’s symbol table
  const char *name;

  union {

    // fields that are relevant for latches
    struct {

      /// current state of this latch
      uint64_t current;

      /// next state of this latch
      uint64_t next;

    };

    // fields that are relevant for AND gates
    struct {

      /// output index of this AND gate
      uint64_t lhs;

      /// input indices of this AND gate
      uint64_t rhs[2];

      /// whether each input is negated
      bool negated[2];

    };
  };
};

/** lookup the input with the given index in this AIG
 *
 * \param aig AIG data structure to search
 * \param index Index of the sought input
 * \param result [out] The located input if successful
 * \returns 0 on success or an errno on failure
 */
int aig_get_input(aig_t *aig, uint64_t index, struct aig_node *result);

/** lookup the latch with the given index in this AIG
 *
 * \param aig AIG data structure to search
 * \param index Index of the sought latch
 * \param result [out] The located latch if successful
 * \returns 0 on success or an errno on failure
 */
int aig_get_latch(aig_t *aig, uint64_t index, struct aig_node *result);

/** lookup the output with the given index in this AIG
 *
 * \param aig AIG data structure to search
 * \param index Index of the sought output
 * \param result [out] The located output if successful
 * \returns 0 on success or an errno on failure
 */
int aig_get_output(aig_t *aig, uint64_t index, struct aig_node *result);

/** lookup the AND gate with the given index in this AIG
 *
 * \param aig AIG data structure to search
 * \param index Index of the sought AND gate
 * \param result [out] The located AND gate if successful
 * \returns 0 on success or an errno on failure
 */
int aig_get_and(aig_t *aig, uint64_t index, struct aig_node *result);

////////////////////////////////////////////////////////////////////////////////

#ifdef __cpluscplus
}
#endif
