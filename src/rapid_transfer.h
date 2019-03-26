#ifndef __RAPID_TRANSFER_H__
#define __RAPID_TRANSFER_H__

#include "tree.h"




/* Compute the Transfer Index comparing a reference tree to an alternative
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

/* Follow a leaf in ref_tree up to the root.  Call add_leaf on the leaves in the
subtrees off the path.
*/
void add_heavy_path(Node* u, Tree* alt_tree);
/* Follow a leaf in ref_tree up to the root. Call reset_leaf on the leaves in
the subtrees off the path.
*/
void reset_heavy_path(Node* u);

/* Add the given leaf (from alt_tree) to the set L(v) for all v on a path from
leaf to the root.
*/
void add_leaf(Node *leaf);
/* Reset the d_min, d_lazy, and diff values for the path from the given leaf
(from alt_tree) to the root.
*/
void reset_leaf(Node *leaf);

/* Follow the nodes on the path, updating d_min on the way up.
*/
void update_dmin_on_path(Node** path, int pathlength);



/* Die if the given node is not a leaf.
*/
void assert_is_leaf(Node* leaf);

/* Return the minimum of two integers.
*/
int min(int i1, int i2);
/* Return the minimum of three integers.
*/
int min3(int i1, int i2, int i3);

/* Return a path (array of Node*) from this node to the root.

@warning  user responsible for the memory.
*/
Node** path_to_root(Node *n);

/* Return an array mapping the index of a leaf Node in leaves1, to a leaf Node
from leaves2.

@warning  modifies (sorts) leaves1 and leaves2
*/
Node** leaf_to_leaf(Node **leaves1, Node **leaves2, int n);

#endif