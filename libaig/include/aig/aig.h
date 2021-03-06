#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
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

// AIG create/delete functions /////////////////////////////////////////////////

/** allocate and initialise a new, blank AIG
 *
 * \param aig [out] Handle to the initialised data structure
 * \param options Configuration for setting up this AIG
 * \returns 0 on success or an errno on failure
 */
int aig_new(aig_t **aig, struct aig_options options);

/** allocate a new AIG and initialise it from the given file
 *
 * \param aig [out] Handle to the initialised data structure
 * \param filename Source file to read
 * \param options Configuration for parsing
 * \returns 0 on success or an errno on failure
 */
int aig_load(aig_t **aig, const char *filename, struct aig_options options);

/** allocate a new AIG and initialise it from the given file pointer
 *
 * \param aig [out] Handle to the initialised data structure
 * \param f Source file handle to read from
 * \param options Configuration for parsing
 * \returns 0 on success or an errno on failure
 */
int aig_loadf(aig_t **aig, FILE *f, struct aig_options options);

/** allocate a new AIG and initialise it from the given string
 *
 * \param aig [out] Handle to the initialised data structure
 * \param content AIG data to read and parse
 * \param options Configuration for parsing
 * \returns 0 on success or an errno on failure
 */
int aig_parse(aig_t **aig, const char *content, struct aig_options options);

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
  AIG_CONSTANT,
  AIG_INPUT,
  AIG_LATCH,
  AIG_OUTPUT,
  AIG_AND_GATE,
};

/// a node in an AIG
struct aig_node {

  /// type of this node
  enum aig_node_type type;

  union {

    // fields that are relevant for constants
    struct {

      /// is this the constant TRUE (as opposed to FALSE)?
      bool is_true;

    } constant;

    // fields that are relevant for inputs
    struct {

      /// variable index in its containing AIG
      uint64_t variable_index;

      /// optional name from the AIG’s symbol table
      const char *name;

    } input;

    // fields that are relevant for latches
    struct {

      /// variable index in its containing AIG
      uint64_t current;

      /// index of next state of this latch
      uint64_t next;

      /// whether there is an inverter
      bool next_negated;

      /// optional name from the AIG’s symbol table
      const char *name;

    } latch;

    // fields that are relevant for outputs
    struct {

      /// index of the output
      uint64_t variable_index;

      /// where it is inverted
      bool negated;

      /// optional name from the AIG’s symbol table
      const char *name;

    } output;

    // fields that are relevant for AND gates
    struct {

      /// output index
      uint64_t lhs;

      /// input indices of this AND gate
      uint64_t rhs[2];

      /// whether each input is negated
      bool negated[2];

    } and_gate;
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

/** lookup a node by variable index
 *
 * Rather than giving an index within a particular category of node like
 * aig_get_input(), aig_get_latch(), and aig_get_and(), this function allows you
 * to retrieve any type of node by its variable index.
 *
 * \param aig AIG data structure to search
 * \param variable_index Index of the sought node
 * \param result [out] The located node if successful
 * \returns 0 on success or an errno on failure
 */
int aig_get_node(aig_t *aig, uint64_t variable_index, struct aig_node *result);

/** lookup the input with the given index in this AIG without its symbol
 *
 * This is similar to aig_get_input() but will not attempt to further parse the
 * symbol table from the backing AIG. You will only get symbol information if it
 * is already in memory. This is effectively an optimised, cheaper version of
 * aig_get_input() when you do not need symbol information.
 *
 * \param aig AIG data structure to search
 * \param index Index of the sought input
 * \param result [out] The located input if successful
 * \returns 0 on success or an errno on failure
 */
int aig_get_input_no_symbol(aig_t *aig, uint64_t index,
  struct aig_node *result);

/** lookup the latch with the given index in this AIG without its symbol
 *
 * This is similar to aig_get_latch() but will not attempt to further parse the
 * symbol table from the backing AIG. You will only get symbol information if it
 * is already in memory. This is effectively an optimised, cheaper version of
 * aig_get_latch() when you do not need symbol information.
 *
 * \param aig AIG data structure to search
 * \param index Index of the sought latch
 * \param result [out] The located latch if successful
 * \returns 0 on success or an errno on failure
 */
int aig_get_latch_no_symbol(aig_t *aig, uint64_t index,
  struct aig_node *result);

/** lookup the output with the given index in this AIG without its symbol
 *
 * This is similar to aig_get_output() but will not attempt to further parse the
 * symbol table from the backing AIG. You will only get symbol information if it
 * is already in memory. This is effectively an optimised, cheaper version of
 * aig_get_output() when you do not need symbol information.
 *
 * \param aig AIG data structure to search
 * \param index Index of the sought output
 * \param result [out] The located output if successful
 * \returns 0 on success or an errno on failure
 */
int aig_get_output_no_symbol(aig_t *aig, uint64_t index,
  struct aig_node *result);

/** lookup a node by name
 *
 * \param aig AIG data structure to search
 * \param name Symbol name of the node being sought
 * \param result [out] The located output if successful
 * \returns 0 on success or an errno on failure
 */
int aig_lookup_node(aig_t *aig, const char *name, struct aig_node *result);

/// an opaque handle to an iterator over an AIG’s nodes
typedef struct aig_node_iter aig_node_iter_t;

/** create a new iterator over this AIG’s nodes
 *
 * \param aig The AIG to iterate over
 * \param it [out] A created iterator on success
 * \returns 0 on success or an errno on failure
 */
int aig_iter(aig_t *aig, aig_node_iter_t **it);

/** is this AIG iterator not exhausted?
 *
 * \param it Iterator to examine
 * \returns True if this iterator has more nodes to yield
 */
bool aig_iter_has_next(const aig_node_iter_t *it);

/** retrieve the next node in this iterator’s AIG
 *
 * \param it Iterator to consume
 * \param item [out] The next node in the iteration on success
 * \returns 0 on success or an errno on failure
 */
int aig_iter_next(aig_node_iter_t *it, struct aig_node *item);

/** deallocate an AIG node iterator
 *
 * \param it The iterator to deallocate
 */
void aig_iter_free(aig_node_iter_t **it);

/** create an iterator over fanout nodes from a given node
 *
 * \param aig AIG to iterate over
 * \param node Node from which to find fanouts
 * \param it [out] A created iterator on success
 * \returns 0 on success or an errno on failure
 */
int aig_iter_fanout(aig_t *aig, const struct aig_node *node,
  aig_node_iter_t **it);

/** get the number of nodes taking the given node as an input in this AIG
 *
 * \param aig The containing AIG
 * \param node The node to consider
 * \param count [out] The fanout count of this node on success
 * \returns 0 on success or an errno on failure
 */
int aig_fanout_count(aig_t *aig, const struct aig_node *node, size_t *count);

/** get the level (maximum distance to an input) of a node within an AIG
 *
 * This function assumes the containing AIG is valid and contains no cycles.
 *
 * \param aig The containing AIG
 * \param node The node to examine
 * \param level [out] The level of this node on success
 * \returns 0 on success or an errno on failure
 */
int aig_node_level(aig_t *aig, const struct aig_node *node, size_t *level);

////////////////////////////////////////////////////////////////////////////////

// SAT generation //////////////////////////////////////////////////////////////

/** generate a SAT representation of an AIG
 *
 * \param aig AIG to translate
 * \param sat [out] SAT problem corresponding to the AIG on success
 * \returns 0 on success or an errno on failure
 */
int aig_to_sat_string(aig_t *aig, char **sat);

/** write a SAT representation of an AIG to a file
 *
 * \param aig AIG to translate
 * \param f Output file to write to
 * \returns 0 on success or an errno on failure
 */
int aig_to_sat_file(aig_t *aig, FILE *f);

/** generate a SAT representation of an AIG term
 *
 * \param node Node to translate
 * \param term [out] SAT representation of the corresponding term on success
 * \returns 0 on success or an errno on failure
 */
int aig_node_to_sat_term(const struct aig_node *node, char **term);

/** generate a SAT representation of an AIG
 *
 * \param node Node to translate
 * \param define [out] SAT representation of the corresponding definition on
 *   success
 * \returns 0 on success or an errno on failure
 */
int aig_node_to_sat_define(const struct aig_node *node, char **define);

/** generate a SAT representation of an AIG
 *
 * \param node Node to translate
 * \param constraint [out] SAT representation of the corresponding constraint on
 *   success
 * \returns 0 on success or an errno on failure
 */
int aig_node_to_sat_constraint(const struct aig_node *node, char **constraint);

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
