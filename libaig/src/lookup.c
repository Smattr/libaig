#include <aig/aig.h>
#include "aig_t.h"
#include <assert.h>
#include <errno.h>
#include "infer.h"
#include <limits.h>
#include "parse.h"
#include <stddef.h>
#include <string.h>

int aig_lookup_node(aig_t *aig, const char *name, struct aig_node *result) {

  if (aig == NULL)
    return EINVAL;

  if (name == NULL)
    return EINVAL;

  if (result == NULL)
    return EINVAL;

  // to look something up by name, we need the full symbol table available, so
  // first ensure we have parsed all of it
  int rc = parse_symtab(aig, UINT64_MAX);
  if (rc)
    return rc;

  // now look for a matching entry in the symbol table
  size_t sz = get_symtab_size(aig);
  for (size_t i = 0; i < sz; i++) {

    // is this a match?
    if (aig->symtab != NULL && aig->symtab[i] != NULL
        && strcmp(name, aig->symtab[i]) == 0) {

      // determine what type of node this is

      if (i < aig->input_count)
        return aig_get_input(aig, i, result);
      i -= aig->input_count;

      if (i < aig->latch_count)
        return aig_get_latch(aig, i, result);
      i -= aig->latch_count;

      assert(i < aig->output_count && "illegal index into symtab");
      return aig_get_output(aig, i, result);
    }
  }

  // if we reached here, we did not find a matching symbol
  return ENOENT;
}
