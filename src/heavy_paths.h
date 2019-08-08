#ifndef __HEAVY_PATHS_H__
#define __HEAVY_PATHS_H__
/*
  This is the code that uses heavy paths to help when the alternative tree is
  not balanced. We use bookkeeping, instead of a minheap, in order to shave a
  logarithmic factor off of the running time when travelling up the alternate
  tree from leaf to root.
 
  A heavy path is split into a tree ("Path Tree" or PT) where each
  node of the tree represents a subpath of the heavypath. Each subpath knows
  its min/max value on the subpath, and its min/max value for all the
  subtrees hanging off the subpath. This way, when a heavy path is updated
  the min value on the path will decrease by 1, but the min value off the
  path increase by 1.

  The PTs are joined together into a "HeavyPath Tree" (HPT), where a root of
  a PT is connected to a leaf of another PT using the parent_heavypath and
  child_heavypath pointers.
*/

#include "tree.h"
#include "debug.h"


typedef struct __Node Node;

/*
  We use the following conventions:
  - the original alt_tree is decomposed into heavypaths
  - a heavypath is represented by a binary tree of Path objects where:
    * the root of the tree contains information for the entire heavpath
    * the leaves of the tree contain node information for the corresponding
      node in alt_tree, along with a pointer to the pendant heavpath

  In other words, a tree of Paths represents a heavypath, where the Path object
  can be an internal node, the root (which contains the bookkeeping for the
  entire heavypath), or a leaf.
  In the case of a leaf, the Path represents the node in alt_tree on the
  current heavypath, pointed to by node, and the pendant heavy path is
  pointed to by child_heavypath.
*/
typedef struct __Path Path;
typedef struct __Path {
  int id;          //A unique identifier.

    //The structure of the heavypath tree:
  Path* left;
  Path* right;
  Path* parent;
  Path* sibling;

  Node* node;               //The node of alt_tree corresponding to this Path.
                            //(this applies only to leaves of the PT)

  Path* child_heavypath;    //The Path tree pendant to this Path.
  Path* parent_heavypath;   //The Path that this PT hangs on.

  int total_depth;          //# of Path structs on the way through all PTs
                            //(through the entire HPT).

    //The transfer index (TI) values:
  int d_lazy;        //The lazily updated transfer distance
  int diff_path;     //Diff to add to subtree rooted on path.
  int diff_subtree;  //Diff to add to pendant subtrees.

  int d_min;         //Minimum TI found in this subtree
  int d_min_path;    //Min value for nodes an the heavypath itself.
  int d_min_subtree; //Min value over all pendant subtrees for this (sub)Path.

  int d_max;         //Maximum TI found in this subtree
  int d_max_path;    //Max value for nodes an the heavypath itself.
  int d_max_subtree; //Max value over all pendant subtrees for this (sub)Path.
} Path;

/*
Allocate a new Path, setting all the default values.
*/
Path* new_Path();

/* Decompose the alternative tree into a heavy paths according to the scheme
described in the definition of the Path struct. Return the root Path of
the Path tree.
*/
Path* heavy_decomposition(Node *root, int depth);

/* For the given heavypath, create a Path structure that represents the path.
Split the path in half and create a Path for each half.  If a half is a single
node, then hang the next heavy path off of it. If it's a leaf of alt_tree, then
link the Path to the corresponding leaf in alt_tree.
*/
Path* partition_heavypath(Node **n, int length, int depth);

/* Return a Path for the given node of alt_tree.  The Path will be a leaf
node of the path tree; either the leaf will point to a leaf node of alt_tree,
or child_heavypath will point to a new Path tree represeting the heavypath
pendant to the alt_tree node.
*/
Path* heavypath_leaf(Node *node, int depth);

/* Return the heavypath rooted at the node.
   ** user responsible for memory of returned heavypath.
*/
Node** get_heavypath(Node* root, int* length);

/* Return the length of the heavy path rooted at this node.
*/
int get_heavypath_length(Node *n);

/* Print the given heavypath.
*/
void print_heavypath(Node **heavypath, int length);


/* Print the Heavy Path Tree (HPT) in dot format.
*/
void print_HPT_dot(Path* hproot, Node* altroot, char* filename);


/* Recursively print the subPath.
*/
void print_HPT_subpath_dot(Path* n, FILE *f);

/* Recursively print the subtree.
*/
void print_HPT_subtree_dot(Node* n, FILE *f);



/* Print a string representing this Path node formatted for dot output.
*/
void print_HPT_node_dot(Path* n, FILE *f);

/* Print a string that formats a heavypath node in a PT.
*/
void print_HPT_hpnode(Path* n, FILE *f);

/* Print a string that formats a PT node.
*/
void print_HPT_ptnode(Path* n, FILE *f);

#endif
