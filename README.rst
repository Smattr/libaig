libaig
======
A library for manipulating `and-inverter graphs`_.

Feature set:

* Parses AIGER_ version 1 ASCII files
* Optimised data structures for minimal memory usage
* Support for on-demand parsing to avoid loading an entire AIG upfront

Example usage:

.. code-block:: c

  #include <aig/aig.h>
  #include <inttypes.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  
  int main(int argc, char **argv) {
  
    // parse an AIG file with default options
    aig_t *aig = NULL;
    int rc = aig_load(&aig, argv[1], (struct aig_options){ 0 });
    if (rc != 0) {
      fprintf(stderr, "aig_load: %s\n", strerror(rc));
      return EXIT_FAILURE;
    }
  
    // print the AIG header
    printf("M = %" PRIu64 "\n", aig_max_index(aig));
    printf("I = %" PRIu64 "\n", aig_input_count(aig));
    printf("L = %" PRIu64 "\n", aig_latch_count(aig));
    printf("O = %" PRIu64 "\n", aig_output_count(aig));
    printf("A = %" PRIu64 "\n", aig_and_count(aig));
  
    // retrieve a particular node
    struct aig_node node;
    rc = aig_get_input(aig, 0, &node);
    if (rc != 0) {
      fprintf(stderr, "aig_get_input: %s\n", strerror(rc));
      return EXIT_FAILURE;
    }
  
    printf("input index = %" PRIu64 "\n", node.input.variable_index);
  
    // or alternatively, create a node iterator...
    aig_node_iter_t *it = NULL;
    rc = aig_iter(aig, &it);
    if (rc != 0) {
      fprintf(stderr, "aig_get_iter: %s\n", strerror(rc));
      return EXIT_FAILURE;
    }
  
    // ...and then iterate through the nodes
    while (aig_iter_has_next(it)) {
  
      struct aig_node n;
      rc = aig_iter_next(it, &n);
      if (rc != 0) {
        fprintf(stderr, "aig_iter_next: %s\n", strerror(rc));
        return EXIT_FAILURE;
      }
  
      // print the node as it is encoded in AIGER format
      switch (n.type) {
  
        // constants are not generated during node iteration
        case AIG_CONSTANT:
          __builtin_unreachable();
  
        case AIG_INPUT:
          printf("%" PRIu64 "\n", n.input.variable_index * 2);
          break;
  
        case AIG_LATCH:
          printf("%" PRIu64 " %" PRIu64 "\n", n.latch.current * 2,
            n.latch.next * 2 + (n.latch.next_negated ? 1 : 0));
          break;
  
        case AIG_OUTPUT:
          printf("%" PRIu64 "\n",
            n.output.variable_index * 2 + (n.output.negated ? 1 : 0));
          break;
  
        case AIG_AND_GATE:
          printf("%" PRIu64 " %" PRIu64 " %" PRIu64 "\n", n.and_gate.lhs * 2,
            n.and_gate.rhs[0] * 2 + (n.and_gate.negated[0] ? 1 : 0),
            n.and_gate.rhs[1] * 2 + (n.and_gate.negated[1] ? 1 : 0));
          break;
  
      }
    }
  
    // clean up
    if (it != NULL)
      aig_iter_free(&it);
    aig_free(&aig);
  
    return EXIT_SUCCESS;
  }

Future road map:

* AIGER version 1.9 and binary support
* Support for writing AIGER files
* From-scratch construction of AIGs in memory

.. _AIGER: http://fmv.jku.at/aiger/
.. _`and-inverter graphs`: https://en.wikipedia.org/wiki/And-inverter_graph
