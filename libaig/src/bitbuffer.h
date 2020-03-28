// abstraction for a dynamically expanding buffer
//
// The buffer API provided below is intended for storing large arrays of numeric
// values. Memory occupancy is a concern when there are many of these live at
// once, so the buffer tightly packs array elements.

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/// Dynamic buffer. A zeroed out structure is considered initialised, so if the
/// buffer or handle members are 0, the buffer is considered empty.
typedef struct __attribute__((visibility("internal"))) {

  /// Main contents of the buffer. Only coherent after fflushing handle.
  char *buffer;
  size_t buffer_size;

  /// Handle to write new items to.
  FILE *handle;

  /// Bits shorter than a byte that were part of the last append.
  uint8_t residue;
  size_t residue_bits;

} bitbuffer_t;

/** append an item to the buffer
 *
 * Items are packed into the backing memory as an array, but the bit width of
 * each item may not be aligned to a number of bytes. The limit parameter is
 * used to determine this bit width for packing.
 *
 * \param bb Buffer to append to
 * \param value Value to append
 * \param limit Largest item value the buffer ever needs to hold
 * \returns 0 on success or an errno on failure
 */
int bb_append(bitbuffer_t *bb, uint64_t value, uint64_t limit);

/** retrieve an item from the buffer
 *
 * \param bb The buffer to read from
 * \param index Index of the item to retrieve
 * \param limit Largest item value this buffer ever needs to hold
 * \param value [out] The value retrieved on success
 * \returns 0 on success or an errno on failure
 */
int bb_get(const bitbuffer_t *bb, uint64_t index, uint64_t limit,
  uint64_t *value);

/** remove all items and clear the state of a buffer
 *
 * After calling this function, all memory associated with the buffer will have
 * been discarded. The buffer can then be reused if desired.
 *
 * \param bb The buffer to operate on
 */
void bb_reset(bitbuffer_t *bb);