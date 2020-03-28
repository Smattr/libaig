// simple application for reading an AIG header, to test libaig

#include <aig/aig.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "usage: %s filename\n", argv[0]);
    return EXIT_FAILURE;
  }

  aig_t *aig = NULL;
  int rc = aig_load(&aig, argv[1], (struct aig_options){ 0 });
  if (rc != 0) {
    perror("aig_load");
    return EXIT_FAILURE;
  }

  printf("M = %" PRIu64 "\n", aig_max_index(aig));
  printf("I = %" PRIu64 "\n", aig_input_count(aig));
  printf("L = %" PRIu64 "\n", aig_latch_count(aig));
  printf("O = %" PRIu64 "\n", aig_output_count(aig));
  printf("A = %" PRIu64 "\n", aig_and_count(aig));

  aig_free(&aig);

  return EXIT_SUCCESS;
}
