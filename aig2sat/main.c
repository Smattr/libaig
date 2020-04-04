// translator from AIG to SAT

#include <aig/aig.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {

  if (argc < 2 || argc > 3) {
    fprintf(stderr, "usage: %s filename [output filename]\n", argv[0]);
    return EXIT_FAILURE;
  }

  aig_t *aig = NULL;
  int rc = aig_load(&aig, argv[1], (struct aig_options){ 0 });
  if (rc != 0) {
    fprintf(stderr, "aig_load: %s\n", strerror(rc));
    return EXIT_FAILURE;
  }

  FILE *out = stdout;
  if (argc > 2) {
    out = fopen(argv[2], "w");
    if (out == NULL) {
      perror("fopen");
      return EXIT_FAILURE;
    }
  }

  if ((rc = aig_to_sat_file(aig, out))) {
    fprintf(stderr, "aig_to_sat_file: %s\n", strerror(rc));
    return EXIT_FAILURE;
  }

  fclose(out);
  aig_free(&aig);

  return EXIT_SUCCESS;
}
