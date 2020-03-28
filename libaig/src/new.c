#include <aig/aig.h>
#include "aig_t.h"
#include <errno.h>
#include <stdlib.h>

int aig_new(aig_t **aig) {

  if (aig == NULL)
    return EINVAL;

  aig_t *a = calloc(1, sizeof(*a));
  if (a == NULL)
    return ENOMEM;

  *aig = a;
  return 0;
}
