// simple application for echoing back an AIG, to test libaig

#include <aig/aig.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "usage: %s filename\n", argv[0]);
    return EXIT_FAILURE;
  }

  aig_t *aig = NULL;
  int rc = aig_load(&aig, argv[1], (struct aig_options){ 0 });
  if (rc != 0) {
    fprintf(stderr, "aig_load: %s\n", strerror(rc));
    return EXIT_FAILURE;
  }

  // print the AIG header
  printf("aag");
  printf(" %" PRIu64, aig_max_index(aig));
  printf(" %" PRIu64, aig_input_count(aig));
  printf(" %" PRIu64, aig_latch_count(aig));
  printf(" %" PRIu64, aig_output_count(aig));
  printf(" %" PRIu64, aig_and_count(aig));
  printf("\n");

  // iterate over the nodes, printing each

  aig_node_iter_t *it = NULL;
  if ((rc = aig_iter(aig, &it)))
    goto done;

  while (aig_iter_has_next(it)) {

    struct aig_node n;
    if ((rc = aig_iter_next(it, &n)))
      goto done;

    switch (n.type) {

      case AIG_INPUT:
        printf("%" PRIu64 "\n", n.variable_index * 2);
        break;

      case AIG_LATCH:
        printf("%" PRIu64 " %" PRIu64 "\n", n.variable_index * 2,
          n.next * 2 + (n.next_negated ? 1 : 0));
        break;

      case AIG_OUTPUT:
        printf("%" PRIu64 "\n", n.output * 2 + (n.output_negated ? 1 : 0));
        break;

      case AIG_AND_GATE:
        printf("%" PRIu64 " %" PRIu64 " %" PRIu64 "\n", n.variable_index * 2,
          n.rhs[0] * 2 + (n.rhs_negated[0] ? 1 : 0),
          n.rhs[1] * 2 + (n.rhs_negated[1] ? 1 : 0));
        break;

    }
  }

done:
  if (it != NULL)
    aig_iter_free(&it);
  aig_free(&aig);

  if (rc != 0)
    fprintf(stderr, "failed: %s\n", strerror(rc));

  return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
