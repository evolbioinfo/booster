/*
Here we implement the faster, tree traversal based, Transfer Index computation.
*/
#include "rapid_transfer.h"


/*
Compute the Transfer Index (TI) for all edges, comparing a reference tree to
an alternative (bootstrap) tree.

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
extern void compute_transfer_indices_new(Tree *ref_tree, const int n,
                                         const int m, Tree *alt_tree,
                                         int *transfer_index)
{
  int* leaf_is = get_leaves(ref_tree); //Indices of leaves in ref_tree->a_nodes
  Node* u;                             //Node we will follow up to the root

  for(int i=0; i < ref_tree->nb_taxa; i++)
  {
    u = ref_tree->a_nodes[leaf_is[i]];     //Start with a leaf
    while(u)                               //Have not visited the root and have
    {                                      //not seen a heavier sibling
      print_node(u);
      Node* parent = u->neigh[0];
      if(u->nneigh == 2 ||                            //u is root
         2*u->subtreesize < parent->subtreesize ||    //u on the light side
         (2*u->subtreesize == parent->subtreesize && is_right_child(u)))
        u = NULL;
      else
        u = u->neigh[0];
    }
  }

  free(leaf_is);
}