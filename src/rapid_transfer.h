#ifndef __RAPID_TRANSFER_H__
#define __RAPID_TRANSFER_H__

#include "tree.h"
#include "debug.h"




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
                                  int *transfer_index, int iteration);

/* Compute the Transfer Index (TI) for all edges, comparing a reference tree to
an alternative balanced (bootstrap) tree.  This does not do a heavypath
decomposition of the aslternative tree.

This is the faster version that is based on assigning an index by traversing
the ref_tree.
We compute the rooted TI using the rooted Transfer Distance (TD). The rooted
TD between node u in ref_tree and node v in alt_tree is the size of symmetric
difference L(u)-L(v) U L(v)-L(u) (where L(u) is the leaf set in the subtree
rooted at u).

We start by computing the TI for a leaf u. Then the TI is computed for parent
u', as long as u is the "heavy" child of u'. This is repeated, starting at
each leaf.

At the end, transfer_index[i] will have the transfer index for edge i.
*/
void compute_transfer_indices_new_BALANCED(Tree *ref_tree, const int n,
                                           const int m, Tree *alt_tree,
                                           int *transfer_index);


/* Compute the edge Transfer Index from the child node transfer index.
*/
void nodeTI_to_edgeTI(Tree* ref_tree);

/* Copy the edge Transfer Index values into the given array.
*/
void edgeTI_to_array(Tree *tree, int *transfer_index);

/* Follow a leaf in ref_tree up to the root.  Call add_leaf on the leaves in the
subtrees off the path.

If use_HPT is false, then assume a balanced alt_tree, otherwise use a heavypath
tree on the alt_tree.
*/
void add_heavy_path(Node* u, Tree* alt_tree, int use_HPT);

/* Follow a leaf in ref_tree up to the root. Call reset_leaf on the leaves in
the subtrees off the path.

If use_HPT is false, then assume balanced alt_tree, otherwise update reset
values on the HeavyPath Tree.
*/
void reset_heavy_path(Node* u, int use_HTP);

/* Add the given leaf (from alt_tree) to the set L(v) for all v on a path from
leaf to the root.
*/
void add_leaf(Node *leaf);
/* Reset the d_min, d_max, d_lazy, and diff values for the path from the given
leaf (from alt_tree) to the root.
*/
void reset_leaf(Node *leaf);

/* Follow the nodes on the path, updating d_min and d_max on the way up.
*/
void update_dminmax_on_path(Node** path, int pathlength);



/* Die if the given node is not a leaf.
*/
void assert_is_leaf(Node* leaf);


/* Return a path (array of Node*) from this node to the root.

@warning  user responsible for the memory.
*/
Node** path_to_root(Node *n);

/* Add the given leaf (from alt_tree) to the set L(v) for all v on a path from
leaf to the root.
*/
void add_leaf_HPT(Node* leaf);

/* Return an array mapping the index of a leaf Node in leaves1, to a leaf Node
from leaves2.

@warning  modifies (sorts) leaves1 and leaves2
*/
Node** leaf_to_leaf(Node **leaves1, Node **leaves2, int n);

/* Set the Transfer Index of the edge above this node (in ref_tree).

@warning  assume that ti_min and ti_max are already set.
*/
void set_edge_TI(Node *u, int n);

#endif
