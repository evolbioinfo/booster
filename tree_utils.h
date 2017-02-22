#ifndef _PARSIMONY_UTILS
#define _PARSIMONY_UTILS

#include "tree.h"
#include "hashmap.h"
#include "sort.h"
#include "stats.h"

Tree* gen_random_tree(Tree *tree);
/**
   This function precomputes the esperence of the expected number of parsimony steps
   implied by a bipartition under the hypothesis that the tree is random.
   In Input:
   - The max depth
   - The number of taxa
   - A pointer to a 2D array (given by precompute_steps_probability(int max_depth, int nb_tax)):
      * First dimension : depth
      * Second dimension : steps
      * value : probability of the step at a given depth
   In output : An array with :
   - the depth in index
   - the expected Number of random parsimony steps  
*/

#endif
