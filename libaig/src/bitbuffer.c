#include <assert.h>
#include "bitbuffer.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// if we need to store values up to the given value, how many bits wide does
// each entry need to be?
static size_t entry_width(uint64_t limit) {

  if (limit == 0)
    return 1;

  return sizeof(unsigned long long) * 8 - __builtin_clzll(limit);
}

int bb_append(bitbuffer_t *bb, uint64_t value, uint64_t limit) {

  assert(value <= limit
    && "attempt to store an out-of-range value in a bit buffer");

  // if the buffer has not been initialised, we need to create it now
  if (bb->handle == NULL) {
    bb->handle = open_memstream(&bb->buffer, &bb->buffer_size);
    if (bb->handle == NULL)
      return errno;
  }

  size_t w = entry_width(limit);

  // write the entry bit-by-bit
  // TODO: accelerate this by writing byte-by-byte
  for (size_t i = 0; i < w; i++) {
    uint64_t bit = (value >> i) & 1;

    assert(bb->residue_bits < 8 && "incurred 1+ bytes in bit buffer's residue");

    // add this bit
    bb->residue |= bit << bb->residue_bits;
    bb->residue_bits++;

    // if we have a full byte, flush this to the buffer
    if (bb->residue_bits == 8) {
      size_t s = fwrite(&bb->residue, 1, 1, bb->handle);
      if (s != 1)
        return ENOMEM;

      bb->residue = 0;
      bb->residue_bits = 0;
    }
  }

  return 0;
}

int bb_get(const bitbuffer_t *bb, uint64_t index, uint64_t limit,
    uint64_t *value) {

  assert(bb != NULL);
  assert(value != NULL);

  // is this bit buffer empty?
  if (bb == NULL)
    return ERANGE;
  if (bb->handle == NULL)
    return ERANGE;

  // synchronise the buffer so we can access bb->buffer
  fflush(bb->handle);

  size_t w = entry_width(limit);

  // does this entry lie beyond the extent of the buffer?
  if (index * w + w > bb->buffer_size * w + bb->residue_bits)
    return ERANGE;

  // read out the entry
  // TODO: accelerate this by doing it byte-by-byte
  uint64_t v = 0;
  for (size_t i = 0; i < w; i++) {
    size_t byte_offset = (index * w + i) / 8;
    size_t bit_offset = (index * w + i) % 8;

    // extract the current bit
    uint8_t byte = 0;
    if (byte_offset < bb->buffer_size) {
      byte = (uint8_t)bb->buffer[byte_offset];
    } else {
      assert(byte_offset == bb->buffer_size &&
        "offset in bb_get() beyond the buffer and the residue byte");
      byte = bb->residue;
    }
    uint64_t bit = (byte >> bit_offset) & 1;

    // add it to the accumulated value
    v = (v << 1) | bit;
  }

  *value = v;
  return 0;
}

void bb_reset(bitbuffer_t *bb) {

  if (bb == NULL)
    return;

  if (bb->handle)
    (void)fclose(bb->handle);

  free(bb->buffer);

  memset(bb, 0, sizeof(*bb));
}
