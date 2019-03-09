#ifndef __RAPID_TRANSFER_H__
#define __RAPID_TRANSFER_H__

#include "tree.h"


/*
Compute the Transfer Index comparing a reference tree to an alternative
(bootstrap) tree.

transfer_index[i] will have the transfer index for edge i.
*/
extern void compute_transfer_indices_new(Tree *ref_tree, const int n,
                                         const int m, Tree *alt_tree,
                                         int *transfer_index);


#endif