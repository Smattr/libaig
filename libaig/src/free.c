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

  if (a->source != NULL) {
    (void)fclose(a->source);
    a->source = NULL;
  }

  free(*aig);
  *aig = NULL;
}
