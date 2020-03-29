#include <aig/aig.h>
#include "aig_t.h"
#include "bitbuffer.h"
#include <stdio.h>
#include <stdlib.h>

void aig_free(aig_t **aig) {

  // allow freeing NULL with no consequences
  if (aig == NULL)
    return;

  aig_t *a = *aig;

  bb_reset(&a->latches);
  bb_reset(&a->outputs);
  bb_reset(&a->ands);

  if (a->source != NULL) {
    (void)fclose(a->source);
    a->source = NULL;
  }

  free(*aig);
  *aig = NULL;
}
