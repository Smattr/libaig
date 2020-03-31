#include <aig/aig.h>
#include "aig_t.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int load(aig_t *aig) {

  assert(aig != NULL);
  assert(aig->source != NULL);

  int rc = 0;

  if ((rc = parse_header(aig)))
    return rc;

  aig->state = IN_INPUTS;
  aig->index = 0;

  // the binary format requires the maximum variable index to be precise, so
  // fail in strict mode if it is not
  if (aig->strict && aig->binary) {
    if (aig->max_index != aig->input_count + aig->latch_count + aig->and_count)
      return ERANGE;
  }

  // if we are parsing lazily, leave the remainder of the file to later
  if (!aig->eager)
    return rc;

  // parse the other sections in the file
  if ((rc = parse_all(aig)))
    return rc;

  return rc;
}

int aig_load(aig_t **aig, const char *filename, struct aig_options options) {

  if (aig == NULL)
    return EINVAL;

  if (filename == NULL)
    return EINVAL;

  FILE *f = fopen(filename, "r");
  if (f == NULL)
    return errno;

  int rc = aig_loadf(aig, f, options);

  if (rc)
    fclose(f);

  return rc;
}

int aig_loadf(aig_t **aig, FILE *f, struct aig_options options) {

  if (aig == NULL)
    return EINVAL;

  if (f == NULL)
    return EINVAL;

  int rc = 0;

  aig_t *a = NULL;
  if ((rc = aig_new(&a, options)))
    goto done;

  a->source = f;
  a->strict = options.strict;
  a->eager = options.eager;

  if ((rc = load(a)))
    goto done;

done:
  if (rc == 0) {
    *aig = a;
  } else {
    // if we got as far as adding the file handle the data structure, remove it
    // so it is not closed by aig_free()
    if (a != NULL && a->source != NULL)
      a->source = NULL;
    aig_free(&a);
  }

  return rc;
}

int aig_parse(aig_t **aig, const char *content, struct aig_options options) {

  if (aig == NULL)
    return EINVAL;

  if (content == NULL)
    return EINVAL;

  FILE *f = fmemopen((void*)content, strlen(content), "r");
  if (f == NULL)
    return errno;

  int rc = aig_loadf(aig, f, options);

  if (rc)
    fclose(f);

  return rc;
}
