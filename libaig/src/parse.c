#include <aig/aig.h>
#include "aig_t.h"
#include "bitbuffer.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "parse.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

static int read_char(FILE *f, char *out) {

  assert(f != NULL);

  int c = getc(f);
  if (c == EOF) {
    int err = ferror(f);
    return err == 0 ? EILSEQ : err;
  }

  *out = c;
  return 0;
}

static int skip(FILE *f, char e) {

  assert(f != NULL);

  char c;
  int err = read_char(f, &c);
  if (err != 0)
    return err;

  if (c != e) {
    ungetc(c, f);
    return EILSEQ;
  }

  return 0;
}

static int skip_space(FILE *f) {
  return skip(f, ' ');
}

static int skip_newline(FILE *f) {
  return skip(f, '\n');
}

static int skip_whitespace(FILE *f) {
  bool read_one = false;
  for (; ; read_one = true) {

    char c;
    int rc = read_char(f, &c);
    if (rc) {
      if (!read_one)
        return rc;
      break;
    }

    if (!isspace(c)) {
      ungetc(c, f);
      break;
    }
  }

  return 0;
}

static int parse_num(FILE *f, uint64_t *out) {

  assert(f != NULL);
  assert(out != NULL);

  int rc = 0;

  char c;
  if ((rc = read_char(f, &c)))
    return rc;

  if (!isdigit(c)) {
    ungetc(c, f);
    return EILSEQ;
  }

  uint64_t v = c - '0';

  for (;;) {

    char d;
    if ((rc = read_char(f, &d))) {
      if (rc == EILSEQ) {
        rc = 0;
        break;
      }
      return rc;
    }

    if (!isdigit(d)) {
      ungetc(d, f);
      break;
    }

    // would the upcoming arithmetic overflow?
    if ((UINT64_MAX - (uint64_t)(d - '0')) / 10 < v) {
      ungetc(d, f);
      return EOVERFLOW;
    }

    v = v * 10 + (d - '0');
  }

  *out = v;
  return rc;
}

int parse_header(aig_t *aig) {

  assert(aig != NULL);
  assert(aig->source != NULL);

  int rc = 0;

  // in non-strict mode, ignore any leading white space
  if (!aig->strict)
    (void)skip_whitespace(aig->source);

  // the header should start with either "aag" for the ASCII format or "aig" for
  // the binary format

  char c;
  if ((rc = read_char(aig->source, &c)))
    return rc;

  if (c != 'a') {
    ungetc(c, aig->source);
    return EILSEQ;
  }

  if ((rc = read_char(aig->source, &c)))
    return rc;

  if (c == 'a') {
    aig->binary = 0;
  } else if (c == 'i') {
    aig->binary = 1;
  } else {
    ungetc(c, aig->source);
    return EILSEQ;
  }

  if ((rc = read_char(aig->source, &c)))
    return rc;

  if (c != 'g') {
    ungetc(c, aig->source);
    return EILSEQ;
  }

  // in non-strict mode, skip arbitrary amounts of white space instead of just a
  // single space
  int (*skipper)(FILE*) = aig->strict ? skip_space : skip_whitespace;

  if ((rc = skipper(aig->source)))
    return rc;

  // now the M, I, L, O, A fields follow

  if ((rc = parse_num(aig->source, &aig->max_index)))
    return rc;

  if ((rc = skipper(aig->source)))
    return rc;

  if ((rc = parse_num(aig->source, &aig->input_count)))
    return rc;

  if ((rc = skipper(aig->source)))
    return rc;

  if ((rc = parse_num(aig->source, &aig->latch_count)))
    return rc;

  if ((rc = skipper(aig->source)))
    return rc;

  if ((rc = parse_num(aig->source, &aig->output_count)))
    return rc;

  if ((rc = skipper(aig->source)))
    return rc;

  if ((rc = parse_num(aig->source, &aig->and_count)))
    return rc;

  if ((rc = aig->strict ? skip_newline(aig->source)
                        : skip_whitespace(aig->source)))
    return rc;

  return rc;
}

int parse_inputs(aig_t *aig, uint64_t upto) {

  assert(aig != NULL);

  // have we already read past the given index?
  if (aig->state > IN_INPUTS || aig->index > upto)
    return 0;

  // for a binary AIG, inputs are omitted
  if (aig->binary)
    return 0;

  for (uint64_t i = aig->index; i < aig->input_count + 1 && i <= upto; i++) {

    // in non-strict mode, ignore leading white space
    if (!aig->strict)
      (void)skip_whitespace(aig->source);

    // parse the input itself
    uint64_t n;
    int rc = parse_num(aig->source, &n);
    if (rc)
      return rc;

    // we already know what it should be, so fail in strict mode if there is a
    // mismatch
    if (aig->strict && n != 2 * i)
      return EILSEQ;

    rc = aig->strict ? skip_newline(aig->source) : skip_whitespace(aig->source);
    if (rc)
      return rc;

    aig->index = i;
  }

  return 0;
}

int parse_latches(aig_t *aig, uint64_t upto) {

  assert(aig != NULL);

  // if we have not yet parsed the input section, we need to first do that
  if (aig->state == IN_INPUTS) {
    int rc = parse_inputs(aig, UINT64_MAX);
    if (rc)
      return rc;
    aig->state = IN_LATCHES;
    aig->index = aig->input_count + 1;
  }

  // have we already read past the given index?
  if (aig->state > IN_LATCHES)
    return 0;
  if (aig->state == IN_LATCHES && aig->index > upto)
    return 0;

  for (uint64_t i = aig->index;
       i < aig->input_count + aig->latch_count + 1 && i <= upto;
       i++) {

    // in non-strict mode, ignore leading white space
    if (!aig->strict)
      (void)skip_whitespace(aig->source);

    // the current state of the latch is only present in the ASCII format
    if (!aig->binary) {

      // parse the current state of the latch
      uint64_t n;
      int rc = parse_num(aig->source, &n);
      if (rc)
        return rc;

      // we already know what the current state should be, so fail in strict
      // mode if there is a mismatch
      if (aig->strict && n != 2 * i)
        return EILSEQ;
    }

    int rc = aig->strict ? skip_space(aig->source)
                         : skip_whitespace(aig->source);
    if (rc)
      return rc;

    // read the next state of the latch
    uint64_t next;
    if ((rc = parse_num(aig->source, &next)))
      return rc;

    // store the parsed value in the latch array
    if ((rc = bb_append(&aig->latches, next, aig->max_index)))
      return rc;

    aig->index = i;
  }

  return 0;
}
