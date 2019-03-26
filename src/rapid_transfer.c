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
  //print_nodes_post_order(ref_tree);
  //fprintf(stderr, "alt_tree:\n");
  //print_nodes_post_order(alt_tree);
  Node** ref_leaves = ref_tree->leaves->a;   //Leaves in ref_tree

    //Compute the TI for each node, following paths from leaves in ref_tree up
    //to the root, calling add_leaf on leaves from pendant subtrees. At the end
    //of each loop, the TI will be the value of d_min at the root of alt_tree.
  Node* u;                                 //Node to follow up to the root
  for(int i=0; i < ref_tree->nb_taxa; i++)
  {
    u = ref_leaves[i];                     //Start with a leaf
    //fprintf(stderr, "------------------ new heavy -------------------------\n");
    //print_node(u);
    //print_nodes_TIvars(alt_tree->a_nodes, alt_tree->nb_nodes);
    add_heavy_path(u, alt_tree);   //Compute TI on heavy path starting at u.
    reset_heavy_path(u);           //Reset TI associated variables on alt_tree.
  }

//fprintf(stderr, "||||||| RESULTS: |||||||\n");
//print_nodes_TI(ref_tree->a_nodes, ref_tree->nb_nodes);
}


/*
Follow a leaf in ref_tree up to the root.  Call add_leaf on the leaves in the
subtrees off the path.
*/
void add_heavy_path(Node* u, Tree* alt_tree)
{
  while(u)                               //Have not visited the root and have
  {                                      //not seen a heavier sibling
    //fprintf(stderr, "________\n");
    //fprintf(stderr, "+++++ "); print_node(u);
      //Add the leaves from the light subtree:
    if(u->nneigh == 1)       //a leaf
      add_leaf(u->other);    //call add_leaf on v
    else
    {
      //fprintf(stderr, "following "); printLA(u->light_leaves);
      for(int i=0; i < u->light_leaves->i; i++)
        add_leaf(u->light_leaves->a[i]->other);
    }

    u->transfer_index = alt_tree->node0->d_min;
    //fprintf(stderr, "+++++ TI: %i\n", u->transfer_index);

      //Head upwards:
    Node* parent = u->neigh[0];
    if(u->depth == 0 ||                             //u is root
       2*u->subtreesize < parent->subtreesize ||    //u on the light side
       (2*u->subtreesize == parent->subtreesize && is_right_child(u)))
      u = NULL;
    else
      u = u->neigh[0];         //u in on the heavy side of its parent.
  }
  //fprintf(stderr, ".....done.....\n");
}

/*
Follow a leaf in ref_tree up to the root. Call reset_leaf on the leaves in
the subtrees off the path.
*/
void reset_heavy_path(Node* u)
{
  while(u)                               //Have not visited the root and have
  {                                      //not seen a heavier sibling
      //Add the leaves from the light subtree:
    if(u->nneigh == 1)       //a leaf
      reset_leaf(u->other);  //call add_leaf on v
    else
      for(int i=0; i < u->light_leaves->i; i++)
        reset_leaf(u->light_leaves->a[i]->other);

      //Head upwards:
    Node* parent = u->neigh[0];
    if(u->depth == 0 ||                             //u is root
       2*u->subtreesize < parent->subtreesize ||    //u on the light side
       (2*u->subtreesize == parent->subtreesize && is_right_child(u)))
      u = NULL;
    else
      u = u->neigh[0];         //u in on the heavy side of its parent.
  }
}


/*
Add the given leaf (from alt_tree) to the set L(v) for all v on a path from
leaf to the root.
*/
void add_leaf(Node *leaf)
{
  assert_is_leaf(leaf);

    //Follow the path from the root to the leaf, updating the d_lazy values
    //with the diff values, subtracting 1, but adding 1 to the diff values of
    //nodes off the path:
  Node **path = path_to_root(leaf);
  for(int i = leaf->depth; i >= 0; i--)
  {
    //fprintf(stderr, "current: "); print_node(path[i]);
    //fprintf(stderr, "         "); print_node_TIvars(path[i]);
    path[i]->d_lazy += path[i]->diff - 1;
    if(i != 0)                               //Not the leaf
    {
      //fprintf(stderr, "         sib "); print_node(path[i-1]->sibling);
      path[i-1]->diff += path[i]->diff;              //Push difference down
      path[i-1]->sibling->diff += path[i]->diff+1;   //the node off the path.
    }
    path[i]->diff = 0;
    //fprintf(stderr, "         "); print_node_TIvars(path[i]);
  }

    //Follow the path back up to the root, updating the d_min values for each
    //pair of siblings:
  update_dmin_on_path(path, leaf->depth+1);
  free(path);
}


/*
Reset the d_min, d_lazy, and diff values for the path from the given leaf
(from alt_tree) to the root.
*/
void reset_leaf(Node *leaf)
{
  assert_is_leaf(leaf);

    //Follow the path from the root to the leaf, reseting the values along
    //the way:
  Node *n = leaf;
  int pathlen = n->depth;
  for(int i=0; i <= pathlen; i++)
  while(1)              //While not the root
  {
    n->d_lazy = n->subtreesize;
    n->d_min = 1;
    n->diff = 0;

    if(n->depth != 0)   //Not the root
    {
      n->sibling->diff = 0;
      n = n->neigh[0];
    }
    else
      break;
  }
}

/*
Follow the nodes on the path, updating d_min on the way up.
*/
void update_dmin_on_path(Node** path, int pathlength)
{
  path[0]->d_min = path[0]->d_lazy;
  for(int i = 1; i < pathlength; i++)
  {
    Node *sib = path[i-1]->sibling;  //The node off the path
    path[i]->d_min = min3(path[i-1]->d_min, sib->d_min+sib->diff,
                          path[i]->d_lazy);
    //fprintf(stderr, "up: "); print_node(path[i]);
  }
}

/*
Die if the given node is not a leaf.
*/
void assert_is_leaf(Node* leaf)
{
  if(leaf->nneigh != 1)   //not a leaf
  {
	  fprintf(stderr, "Error: leaf not given to add_leaf().\n");
	  Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
  }
}


/* Return the minimum of two integers.
*/
int min(int i1, int i2)
{
  if(i1 < i2)
    return i1;
  return i2;
}
/* Return the minimum of three integers.
*/
int min3(int i1, int i2, int i3)
{
  return min(min(i1, i2), i3);
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
