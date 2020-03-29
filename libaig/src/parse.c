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

  for (uint64_t i = aig->index; i < aig->input_count && i <= upto; i++) {

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
    if (aig->strict && n != 2 * (i + 1))
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
    aig->index = 0;
  }

  // have we already read past the given index?
  if (aig->state > IN_LATCHES)
    return 0;
  if (aig->state == IN_LATCHES && aig->index > upto)
    return 0;

  for (uint64_t i = aig->index; i < aig->latch_count && i <= upto; i++) {

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
      if (aig->strict && n != 2 * (i + 1 + aig->input_count))
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

    // fail if this exceeds the maximum variable index, as we rely on this to
    // store the next states in aig->latches
    if (next > bb_limit(aig))
      return ERANGE;

    // read the line terminator
    rc = aig->strict ? skip_newline(aig->source) : skip_whitespace(aig->source);
    if (rc)
      return rc;

    // store the parsed value in the latch array
    if ((rc = bb_append(&aig->latches, next, bb_limit(aig))))
      return rc;

    aig->index = i;
  }

  return 0;
}

int parse_outputs(aig_t *aig, uint64_t upto) {

  assert(aig != NULL);

  // if we have not yet parsed inputs and latches, we need to first parse those
  // sections
  if (aig->state < IN_OUTPUTS) {
    int rc = parse_latches(aig, UINT64_MAX);
    if (rc)
      return rc;
    aig->state = IN_OUTPUTS;
    aig->index = 0;
  }

  // have we already read past the given index?
  if (aig->state > IN_OUTPUTS)
    return 0;
  if (aig->state == IN_OUTPUTS && aig->index > upto)
    return 0;

  for (uint64_t i = aig->index; i < aig->output_count && i <= upto; i++) {

    // in non-strict mode, ignore leading white space
    if (!aig->strict)
      (void)skip_whitespace(aig->source);

    // parse the current output
    uint64_t o;
    int rc = parse_num(aig->source, &o);
    if (rc)
      return rc;

    // is the output a legal variable index?
    if (o > bb_limit(aig))
      return ERANGE;

    // read the line terminator
    rc = aig->strict ? skip_newline(aig->source) : skip_whitespace(aig->source);
    if (rc)
      return rc;

    // store the parsed value in the output array
    if ((rc = bb_append(&aig->outputs, o, bb_limit(aig))))
      return rc;

    aig->index = i;
  }

  return 0;
}

static int parse_and_ascii(aig_t *aig, uint64_t index) {

  assert(aig != NULL);

  // in non-strict mode, ignore leading white space
  if (!aig->strict)
    (void)skip_whitespace(aig->source);

  // parse the index of the AND gate
  uint64_t lhs;
  int rc = parse_num(aig->source, &lhs);
  if (rc)
    return rc;

  // we already know the expected index, so fail in strict mode if this
  // mismatches
  if (aig->strict) {
    if (lhs != 2 * (index + aig->input_count + aig->latch_count + 1))
      return EILSEQ;
  }

  rc = aig->strict ? skip_space(aig->source) : skip_whitespace(aig->source);
  if (rc)
    return rc;

  // read the first operand
  uint64_t rhs0;
  if ((rc = parse_num(aig->source, &rhs0)))
    return rc;

  // is this an encoding of a legal variable index?
  if (rhs0 > bb_limit(aig))
    return ERANGE;

  rc = aig->strict ? skip_space(aig->source) : skip_whitespace(aig->source);
  if (rc)
    return rc;

  // read the second operand
  uint64_t rhs1;
  if ((rc = parse_num(aig->source, &rhs1)))
    return rc;

  // is this an encoding of a legal variable index?
  if (rhs1 > bb_limit(aig))
    return ERANGE;

  // read the line terminator
  rc = aig->strict ? skip_newline(aig->source) : skip_whitespace(aig->source);
  if (rc)
    return rc;

  // store these values in the AND gates array
  if ((rc = bb_append(&aig->ands, rhs0, bb_limit(aig))))
    return rc;
  if ((rc = bb_append(&aig->ands, rhs1, bb_limit(aig))))
    return rc;

  return 0;
}

static int parse_and_binary(aig_t *aig, uint64_t index) {
  (void)aig;
  (void)index;
  return ENOTSUP;
}

static int parse_and(aig_t *aig, uint64_t index) {
  return aig->binary ? parse_and_binary(aig, index)
                     : parse_and_ascii(aig, index);
}

int parse_ands(aig_t *aig, uint64_t upto) {

  // if we have not yet parsed inputs, latches, and outputs we need to first
  // parse those sections
  if (aig->state < IN_ANDS) {
    int rc = parse_outputs(aig, UINT64_MAX);
    if (rc)
      return rc;
    aig->state = IN_ANDS;
    aig->index = 0;
  }

  // have we already read past the given index?
  if (aig->state > IN_ANDS)
    return 0;
  if (aig->state == IN_ANDS && aig->index > upto)
    return 0;

  for (uint64_t i = aig->index; i < aig->and_count && i <= upto; i++) {
    int rc = parse_and(aig, i);
    if (rc)
      return rc;
    aig->index = i;
  }

  return 0;
}
