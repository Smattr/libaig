#include <aig/aig.h>
#include "aig_t.h"
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int aig_to_sat_string(aig_t *aig, char **sat) {

  if (aig == NULL)
    return EINVAL;

  if (sat == NULL)
    return EINVAL;

  // create a buffer to write the SAT representation into
  char *out = NULL;
  size_t out_size = 0;
  FILE *f = open_memstream(&out, &out_size);
  if (f == NULL)
    return errno;

  // delegate to the FILE interface
  int rc = aig_to_sat_file(aig, f);

  // finalise the buffer
  fclose(f);

  if (rc) {
    free(out);
    return rc;
  }

  *sat = out;
  return 0;
}

/** write the term corresponding to a variable index to the given file
 *
 * \param index Variable index to reference
 * \param f Output stream to write to
 * \returns 0 on success or an errno on failure
 */
static int index_to_sat_term(uint64_t index, FILE *f) {

  assert(f != NULL);

  if (fprintf(f, "s%" PRIu64, index) < 0)
    return errno;

  return 0;
}

/** write the term corresponding to a node to the given file
 *
 * \param node Node to inspect
 * \param f Output stream to write to
 * \returns 0 on success or an errno on failure
 */
static int node_to_sat_term(const struct aig_node *node, FILE *f) {

  assert(node != NULL);
  assert(f != NULL);

  int rc = 0;

  switch (node->type) {

    case AIG_CONSTANT:
      if (node->constant.is_true) {
        if (fprintf(f, "True") < 0)
          return errno;
      } else {
        if (fprintf(f, "False") < 0)
          return errno;
      }
      break;

    case AIG_INPUT:
      if ((rc = index_to_sat_term(node->input.variable_index, f)))
        return rc;
      break;

    case AIG_OUTPUT:
      if ((rc = index_to_sat_term(node->output.variable_index, f)))
        return rc;
      break;

    case AIG_LATCH:
      if ((rc = index_to_sat_term(node->latch.current, f)))
        return rc;
      break;

    case AIG_AND_GATE:
      if ((rc = index_to_sat_term(node->and_gate.lhs, f)))
        return rc;
      break;

  }

  return rc;
}

/** retrieve the name of a node
 *
 * \param node The node to inspect
 * \returns The name of this node or NULL if it has no name
 */
static const char *node_name(const struct aig_node *node) {
  switch (node->type) {
    case AIG_INPUT:  return node->input.name;
    case AIG_LATCH:  return node->latch.name;
    case AIG_OUTPUT: return node->output.name;
    default:         return NULL;
  }
}

/** write the definition corresponding to a node to the given file
 *
 * \param node Node to inspect
 * \param f Output stream to write to
 * \returns 0 on success or an errno on failure
 */
static int node_to_sat_define(const struct aig_node *node, FILE *f) {

  assert(node != NULL);
  assert(f != NULL);

  // True and False do not need to be defined
  if (node->type == AIG_CONSTANT)
    return 0;

  if (fprintf(f, "(declare-fun ") < 0)
    return errno;

  int rc = node_to_sat_term(node, f);
  if (rc)
    return rc;

  if (fprintf(f, " () Bool)") < 0)
    return errno;

  // append the name as a comment, if it exists
  const char *name = node_name(node);
  if (name != NULL) {
    if (fprintf(f, " ; %s", name) < 0)
      return errno;
  }

  if (fprintf(f, "\n") < 0)
    return errno;

  return 0;
}

/** write the constraint corresponding to a node to the given file
 *
 * \param node Node to inspect
 * \param f Output stream to write to
 * \returns 0 on success or an errno on failure
 */
static int node_to_sat_constraint(const struct aig_node *node, FILE *f) {

  assert(node != NULL);
  assert(f != NULL);

  // no constraint for True, False, inputs or outputs
  if (node->type == AIG_CONSTANT)
    return 0;
  if (node->type == AIG_INPUT)
    return 0;
  if (node->type == AIG_OUTPUT)
    return 0;

#define PR(args...) do { if (fprintf(f, args) < 0) return errno; } while (0)

  PR("(assert (= ");

  int rc = 0;
  if ((rc = node_to_sat_term(node, f)))
    return rc;

  PR(" ");

  switch (node->type) {

    case AIG_LATCH:
      if (node->latch.next_negated)
        PR("(not ");
      if ((rc = index_to_sat_term(node->latch.next, f)))
        return rc;
      if (node->latch.next_negated)
        PR(")");
      break;

    case AIG_AND_GATE:
      PR("(and ");
      if (node->and_gate.negated[0])
        PR("(not ");
      if ((rc = index_to_sat_term(node->and_gate.rhs[0], f)))
        return rc;
      if (node->and_gate.negated[0])
        PR(")");
      PR(" ");
      if (node->and_gate.negated[1])
        PR("(not ");
      if ((rc = index_to_sat_term(node->and_gate.rhs[1], f)))
        return rc;
      if (node->and_gate.negated[1])
        PR(")");
      PR(")");
      break;

    default:
      __builtin_unreachable();
  }

  PR("))\n");

#undef PR

  return 0;
}

int aig_to_sat_file(aig_t *aig, FILE *f) {

  if (aig == NULL)
    return EINVAL;

  if (f == NULL)
    return EINVAL;

  int rc = 0;

  // define each node
  {
    aig_node_iter_t *it = NULL;
    if ((rc = aig_iter(aig, &it)))
      return rc;

    while (aig_iter_has_next(it)) {

      struct aig_node n;
      if ((rc = aig_iter_next(it, &n))) {
        aig_iter_free(&it);
        return rc;
      }

      if ((rc = node_to_sat_define(&n, f))) {
        aig_iter_free(&it);
        return rc;
      }
    }

    aig_iter_free(&it);
  }

  // emit constraints for each node
  {
    aig_node_iter_t *it = NULL;
    if ((rc = aig_iter(aig, &it)))
      return rc;

    while (aig_iter_has_next(it)) {

      struct aig_node n;
      if ((rc = aig_iter_next(it, &n))) {
        aig_iter_free(&it);
        return rc;
      }

      if ((rc = node_to_sat_constraint(&n, f))) {
        aig_iter_free(&it);
        return rc;
      }
    }

    aig_iter_free(&it);
  }

  return 0;
}

int aig_node_to_sat_term(const struct aig_node *node, char **term) {

  if (node == NULL)
    return EINVAL;

  if (term == NULL)
    return EINVAL;

  // create a buffer to write the SAT representation into
  char *out = NULL;
  size_t out_size = 0;
  FILE *f = open_memstream(&out, &out_size);
  if (f == NULL)
    return errno;

  // delegate to the FILE interface
  int rc = node_to_sat_term(node, f);

  // finalise the buffer
  fclose(f);

  if (rc) {
    free(out);
    return rc;
  }

  *term = out;
  return 0;
}

int aig_node_to_sat_define(const struct aig_node *node, char **define) {

  if (node == NULL)
    return EINVAL;

  if (define == NULL)
    return EINVAL;

  // create a buffer to write the SAT representation into
  char *out = NULL;
  size_t out_size = 0;
  FILE *f = open_memstream(&out, &out_size);
  if (f == NULL)
    return errno;

  // delegate to the FILE interface
  int rc = node_to_sat_define(node, f);

  // finalise the buffer
  fclose(f);

  if (rc) {
    free(out);
    return rc;
  }

  *define = out;
  return 0;
}

int aig_node_to_sat_constraint(const struct aig_node *node, char **constraint) {

  if (node == NULL)
    return EINVAL;

  if (constraint == NULL)
    return EINVAL;

  // create a buffer to write the SAT representation into
  char *out = NULL;
  size_t out_size = 0;
  FILE *f = open_memstream(&out, &out_size);
  if (f == NULL)
    return errno;

  // delegate to the FILE interface
  int rc = node_to_sat_constraint(node, f);

  // finalise the buffer
  fclose(f);

  if (rc) {
    free(out);
    return rc;
  }

  *constraint = out;
  return 0;
}
