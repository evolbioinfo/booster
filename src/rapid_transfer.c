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

@warning  Assumes that set_leaf_bijection() has been called.
*/
void compute_transfer_indices_new(Tree *ref_tree, const int n,
                                  const int m, Tree *alt_tree,
                                  int *transfer_index)
{
  print_nodes_post_order(ref_tree);
  Node** ref_leaves = ref_tree->leaves->a;   //Leaves in ref_tree

    //Follow a paths from leaves in ref_tree up to the root, calling add_leaf
    //on leaves from pendant subtrees:
  Node* u;                                 //Node to follow up to the root
  for(int i=0; i < ref_tree->nb_taxa; i++)
  {
    u = ref_leaves[i];                     //Start with a leaf
    fprintf(stderr, "-----------------------------------------------------\n");
    while(u)                               //Have not visited the root and have
    {                                      //not seen a heavier sibling
      fprintf(stderr, "________\n");
      print_node(u);
        //Add the leaves from the light subtree:
      if(u->nneigh == 1)       //a leaf
        add_leaf(u->other);
      else
      {
        printLA(u->light_leaves);
        for(int i=0; i < u->light_leaves->i; i++)
        {
          add_leaf(u->light_leaves->a[i]);
        }
      }

        //Head upwards:
      Node* parent = u->neigh[0];
      if(u->depth == 0 ||                             //u is root
         2*u->subtreesize < parent->subtreesize ||    //u on the light side
         (2*u->subtreesize == parent->subtreesize && is_right_child(u)))
      {
        //fprintf(stderr, "stop\n");
        u = NULL;
      }
      else
        u = u->neigh[0];         //u in on the heavy side of its parent.
    }
  }
}


/*
Return an array mapping the index of a leaf Node in leaves1, to a pointer to a
leaf Node from leaves2.

** modifies (sorts) leaves1 and leaves2
*/
Node** leaf_to_leaf(Node **leaves1, Node **leaves2, int n)
{
    //Sort the leaves by their string name:
  qsort(leaves1, n, sizeof(Node*), compare_nodes);
  qsort(leaves2, n, sizeof(Node*), compare_nodes);

    //Compute the mapping:
  Node **indexTOleaf2 = calloc(n, sizeof(Node*));
  for(int i=0; i < n; i++)
    indexTOleaf2[i] = leaves2[i];

  return indexTOleaf2;
}

/*
Add the given leaf to the set L(v) for all v on a path from leaf to the root.

@warning  assume the given leaf is in the tree
*/
void add_leaf(Node *leaf)
{
  if(leaf->nneigh != 1)   //not a leaf
  {
	  fprintf(stderr, "Error: leaf not given to add_leaf().\n");
	  Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
  }

  fprintf(stderr, "addleaf: ");
  print_node(leaf);
  Node **path = path_to_root(leaf);
  for(int i = leaf->depth; i >= 0; i--)
  {
    print_node(path[i]);
  }

  free(path);
}


/*
Return a path (array of Node*) from this node to the root.

@warning  user responsible for the memory.
*/
Node** path_to_root(Node *n)
{
  int pathlen = n->depth;
  Node **path = calloc(n->depth+1, sizeof(Node*));
  for(int i=0; i <= pathlen; i++)
  {
    path[i] = n;
    n = n->neigh[0];  //If depth is set wrong then this will go back to child.
  }

  return path;
}