#ifndef __RAPID_TRANSFER_H__
#define __RAPID_TRANSFER_H__

#include "tree.h"




/*
Compute the Transfer Index comparing a reference tree to an alternative
(bootstrap) tree.

This is the faster version that is based on assigning an index by traversing
the ref_tree.
We compute the rooted TI using the rooted Transfer Distance (TD). The rooted
TD between node u in ref_tree and node v in alt_tree is the size of symmetric
difference L(u)-L(v) U L(v)-L(u) (where L(u) is the leaf set in the subtree
rooted at u).

The transfer index for a node u' in ref_tree is computed from the transfer
index of its child u.  The symmetric difference of leaf sets is kept implicity
in the alt_tree.

At the end, transfer_index[i] will have the transfer index for edge i.
*/
void compute_transfer_indices_new(Tree *ref_tree, const int n,
                                  const int m, Tree *alt_tree,
                                  int *transfer_index);


/*
Return an array mapping the index of a leaf Node in leaves1, to a leaf Node
from leaves2.

** modifies (sorts) leaves1 and leaves2
*/
Node** leaf_to_leaf(Node **leaves1, Node **leaves2, int n);

/*
Add the given leaf to the set L(v) for all v on a path from leaf to the root.
*/
void add_leaf(Node *leaf);

/*
Return a path (array of Node*) from this node to the root.

** user responsible for the memory.
*/
Node** path_to_root(Node *n);


#endif