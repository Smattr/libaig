#include <aig/aig.h>
#include "aig_t.h"
#include <errno.h>
#include <stdlib.h>

int aig_new(aig_t **aig, struct aig_options options) {

  if (aig == NULL)
    return EINVAL;

  aig_t *a = calloc(1, sizeof(*a));
  if (a == NULL)
    return ENOMEM;

  // options.strict is irrelevant when we are not parsing the AIG but rather
  // creating it from scratch, so ignore it
  (void)options;

  *aig = a;
  return 0;
}
