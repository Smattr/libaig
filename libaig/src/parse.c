#include <aig/aig.h>
#include "aig_t.h"
#include "bitbuffer.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "parse.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

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

  // unconditionally strictly require a newline to follow, to ensure we fail to
  // load AIGER 1.9 files for now
  if ((rc = skip_newline(aig->source)))
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

  for (; aig->index < aig->input_count && aig->index <= upto; aig->index++) {
    uint64_t i = aig->index;

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

  int rc = 0;

  for (; aig->index < aig->latch_count && aig->index <= upto; aig->index++) {
    uint64_t i = aig->index;

    // in non-strict mode, ignore leading white space
    if (!aig->strict)
      (void)skip_whitespace(aig->source);

    // the current state of the latch is only present in the ASCII format
    if (!aig->binary) {

      // parse the current state of the latch
      uint64_t n;
      if ((rc = parse_num(aig->source, &n)))
        return rc;

      // we already know what the current state should be, so fail in strict
      // mode if there is a mismatch
      if (aig->strict && n != 2 * (i + 1 + aig->input_count))
        return EILSEQ;

      rc = aig->strict ? skip_space(aig->source)
                       : skip_whitespace(aig->source);
      if (rc)
        return rc;
    }

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

  for (; aig->index < aig->output_count && aig->index <= upto; aig->index++) {

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

  // if this AND gate’s LHS is out of the expected (and inferable) sequence or
  // we have existing LHS data indicating that a prior gate had an LHS out of
  // sequence, we will need to add it to the aig->and_lhs array
  if (lhs != get_inferred_and_lhs(aig, index) || !bb_is_empty(&aig->and_lhs)) {

    // if we have no LHS data, every AND gate before this one had an inferable
    // LHS, so we need to construct all this data now
    if (bb_is_empty(&aig->and_lhs)) {
      for (uint64_t i = 0; i < index; i++) {
        uint64_t l = get_inferred_and_lhs(aig, i);
        if ((rc = bb_append(&aig->and_lhs, l, bb_limit(aig))))
          return rc;
      }
    }

    // store the current LHS
    if ((rc = bb_append(&aig->and_lhs, lhs, bb_limit(aig))))
      return rc;
  }

  // store the RHSs values in the AND gates array
  if ((rc = bb_append(&aig->and_rhs, rhs0, bb_limit(aig))))
    return rc;
  if ((rc = bb_append(&aig->and_rhs, rhs1, bb_limit(aig))))
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

  for (; aig->index < aig->and_count && aig->index <= upto; aig->index++) {
    int rc = parse_and(aig, aig->index);
    if (rc)
      return rc;
  }

  return 0;
}

int parse_symtab(aig_t *aig, uint64_t upto) {

  // if we have not yet parsed the preceding sections, parse those now
  if (aig->state < IN_SYMTAB) {
    int rc = parse_ands(aig, UINT64_MAX);
    if (rc)
      return rc;
    aig->state = IN_SYMTAB;
    aig->index = 0; // index is irrelevant when parsing the symbol table
  }

  // have we already finished parsing the symbol table?
  if (aig->state > IN_SYMTAB)
    return 0;

  size_t symtab_size = aig->input_count + aig->latch_count + aig->output_count;

  // if we are seeking something in range of the symbol table, see if we already
  // have it
  if (upto < symtab_size) {
    if (aig->symtab != NULL && aig->symtab[upto] != NULL)
      return 0;
  }

  // start parsing the symbol table
  for (;;) {

    // in non-strict mode, ignore leading white space
    if (!aig->strict)
      (void)skip_whitespace(aig->source);

    char c;
    int rc = read_char(aig->source, &c);
    if (rc == EILSEQ) { // EOF
      aig->state = DONE;
      return 0;
    } else if (rc) {
      return rc;
    }

    // have we reached the comment section?
    if (c == 'c') {
      aig->state = DONE;
      return 0;
    }

    // is this a illegal category for a symbol?
    if (c != 'i' && c != 'l' && c != 'o') {
      ungetc(c, aig->source);
      return EILSEQ;
    }

    // parse the position of the symbol
    uint64_t pos;
    if ((rc = parse_num(aig->source, &pos)))
      return rc;

    // is the position a illegal index?
    if (c == 'i' && pos >= aig->input_count)
      return ERANGE;
    if (c == 'l' && pos >= aig->latch_count)
      return ERANGE;
    if (c == 'o' && pos >= aig->output_count)
      return ERANGE;

    // skip the space in between the position and the symbol name
    rc = aig->strict ? skip_space(aig->source) : skip_whitespace(aig->source);
    if (rc)
      return rc;

    // we will need to write to the symbol table, so create it now if it has not
    // been allocated
    if (aig->symtab == NULL) {
      aig->symtab = calloc(symtab_size, sizeof(aig->symtab[0]));
      if (aig->symtab == NULL)
        return ENOMEM;
    }

    // set up a buffer for reading the symbol name
    char *buffer = NULL;
    size_t buffer_size = 0;
    FILE *b = open_memstream(&buffer, &buffer_size);
    if (b == NULL)
      return errno;

    // now read the symbol name into the buffer
    char s;
    while (!(rc = read_char(aig->source, &s))) {
      if (s == '\n')
        break;
      if (putc(s, b) == EOF) {
        fclose(b);
        free(buffer);
        return ENOMEM;
      }
    }

    // synchronise the buffer
    fclose(b);

    if (rc) {
      free(buffer);
      return rc;
    }

    // determine where this should lie in the symbol table
    uint64_t index = pos;
    if (c == 'l' || c == 'o')
      index += aig->input_count;
    if (c == 'o')
      index += aig->latch_count;

    assert(index < symtab_size &&
      "out of bounds access when constructing symbol table");

    // if there was a previous name for this, allow it to be overwritten if we
    // are non-strict
    if (aig->symtab[index] != NULL) {
      if (aig->strict) {
        free(buffer);
        return EEXIST;
      }
      free(aig->symtab[index]);
      aig->symtab[index] = NULL;
    }

    // write this entry into the symbol table
    aig->symtab[index] = buffer;

    // if this was the entry we were seeking, we are done
    if (index == upto)
      return 0;
  }

  return 0;
}

int parse_all(aig_t *aig) {
  // because the symbol table is the last section we are interested in and its
  // parsing function calls all the preceding ones, simply ask to parse the
  // entire symbol table
  return parse_symtab(aig, UINT64_MAX);
}
