#include <aig/aig.h>
#include "aig_t.h"
#include "bitbuffer.h"
#include "infer.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

void aig_free(aig_t **aig) {

  // allow freeing NULL with no consequences
  if (aig == NULL)
    return;

  aig_t *a = *aig;

  bb_reset(&a->inputs);
  bb_reset(&a->latch_current);
  bb_reset(&a->latch_next);
  bb_reset(&a->outputs);
  bb_reset(&a->and_lhs);
  bb_reset(&a->and_rhs);

  if (a->symtab != NULL) {
    size_t sz = get_symtab_size(a);
    for (size_t i = 0; i < sz; i++) {
      free(a->symtab[i]);
      a->symtab[i] = NULL;
    }
  }
  free(a->symtab);
  a->symtab = NULL;

  if (a->source != NULL)
    (void)fclose(a->source);
  a->source = NULL;

  free(*aig);
  *aig = NULL;
}
