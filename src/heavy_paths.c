/*
Here we implement the heavy path decomposition of the alt_tree.
*/
#include "heavy_paths.h"

/*
Allocate a new Path, setting all the default values.
*/
int id_counter = 0;          //Global id counter for Path structs.
Path* new_Path()
{
  Path *newpath = malloc(sizeof(Path));

  newpath->id = id_counter++;

  newpath->left = NULL;
  newpath->right = NULL;
  newpath->parent = NULL;
  newpath->sibling = NULL;

  newpath->node = NULL;

  newpath->child_heavypath = NULL;
  newpath->parent_heavypath = NULL;

  newpath->total_depth = 0;

  newpath->d_lazy = 0;
  newpath->diff_path = newpath->diff_subtree = 0;

  newpath->d_min = 1;
  newpath->d_min_path = 1;
  newpath->d_min_subtree = 1;

  newpath->d_max = 0;
  newpath->d_max_path = 0;
  newpath->d_max_subtree = 0;

  return newpath;
}


/*
Recursiveley decompose the alternative tree into heavy paths according to
the scheme described in the definition of the Path struct. Return the root
Path of the Path tree.
*/
Path* heavy_decomposition(Node *root, int depth)
{
  int length;
  Node** heavypath = get_heavypath(root, &length);

  Path *path_root;
  if(length == 1)
    path_root = heavypath_leaf(heavypath[0], depth);
  else
    path_root = partition_heavypath(heavypath, length, depth);

  free(heavypath);

  return path_root;
}

/*
For the given heavypath, create a Path structure that represents the path.
Split the path in half and create a Path for each half.  If a half is a single
node, then hang the next heavy path off of it. If it's a leaf of alt_tree, then
link the Path to the corresponding leaf in alt_tree.
*/
Path* partition_heavypath(Node **heavypath, int length, int depth)
{
  Path* newpath = new_Path();
  newpath->total_depth = depth;

    //Split the heavy path into two equal-length subpaths:
  int l1 = ceil(length/2);
  if(l1 == 1)
    newpath->left = heavypath_leaf(heavypath[0], depth+1);
  else
    newpath->left = partition_heavypath(heavypath, l1, depth+1);
  newpath->left->parent = newpath;

  int l2 = length - l1;
  if(l2 == 1)
    newpath->right = heavypath_leaf(heavypath[l1], depth+1);
  else
    newpath->right = partition_heavypath(&heavypath[l1], l2, depth+1);
  newpath->right->parent = newpath;

  newpath->right->sibling = newpath->left;
  newpath->left->sibling = newpath->right;

  newpath->d_min = min(newpath->left->d_min, newpath->right->d_min);
  newpath->d_max = max(newpath->left->d_max, newpath->right->d_max);

  return newpath;
}

/*
Return a Path for the given node of alt_tree.  The Path will be a leaf
node of the path tree; either the leaf will point to a leaf node of alt_tree,
or child_heavypaths will be an array of heavypaths representing the descendants
of the alt_tree node.
*/
Path* heavypath_leaf(Node *node, int depth)
{
  Path* newpath = new_Path();

  newpath->total_depth = depth;
  newpath->node = node;           //attach the path to the node
  node->path = newpath;           //attach the node to the path

  newpath->d_lazy = node->subtreesize;
  newpath->d_max = newpath->d_max_path = node->subtreesize;

    //Handle an internal alt_tree node with pendant heavypath:
  if(node->nneigh > 1 && node->nneigh <= 3)
  {
    int i_neigh = 1;               //don't look at the parent
    if(node->nneigh == 2)          //If we are the root of alt_tree
      i_neigh = 0;                 //then we have no parent.

    if(node->neigh[i_neigh] != node->heavychild)
    {
      newpath->child_heavypath = heavy_decomposition(node->neigh[i_neigh],
                                                     depth+1);
      newpath->child_heavypath->parent_heavypath = newpath;
    }
    else if(node->neigh[i_neigh+1] != node->heavychild)
    {
      newpath->child_heavypath = heavy_decomposition(node->neigh[i_neigh+1],
                                                     depth+1);
      newpath->child_heavypath->parent_heavypath = newpath;
    }
    else
    {
      fprintf(stderr, "ERROR: Unexpected heavypath configuration!\n");
      Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
    }

    newpath->d_max_subtree = newpath->child_heavypath->d_max;
  }
  else if(node->nneigh > 3)
  {
    fprintf(stderr, "ERROR: This code works for binary trees only.\n");
    Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
  }
  else                              //node is a leaf of alt_tree
  {
    newpath->d_max_subtree = 1;
  }

  return newpath;
}


/*
Return the heavypath rooted at the node.
   - user responsible for memory of returned heavypath.
*/
Node** get_heavypath(Node* root, int* length)
{
  *length = get_heavypath_length(root);

  Node** heavypath = calloc(*length, sizeof(Node*));
  Node* current = root;
  for(int i=0; i < *length; i++)
  {
    heavypath[i] = current;
    current = current->heavychild;
  }

  return heavypath;
}


/*
Return the number of nodes in the heavy path rooted at this node.
*/
int get_heavypath_length(Node *n)
{
    //Get the heavypath length:
  int length = 1;
  while(n->nneigh != 1)      //not a leaf
  {
    length++;
    n = n->heavychild;
  }
  return length;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - I/O - - - - - - - - - - - - - - - - - - - - -

/*
Print the given heavypath.
*/
void print_heavypath(Node **heavypath, int length)
{
  for(int i=0; i < length; i++)
    printf("%i ", heavypath[i]->id);
  printf("\n");
}

/*
Print the Heavy Path Tree (HPT) in dot format.
*/
void print_HPT_dot(Path* hproot, Node* altroot, char* filename)
{
  FILE *f = fopen(filename, "w");
  if(f == NULL)
  {
    fprintf(stderr, "Can't open file for writing!");
    exit(0);
  }

  fprintf(f, "digraph HPT\n  {\n  center=true;\n");
  print_HPT_subpath_dot(hproot, f);
  print_HPT_subtree_dot(altroot, f);
  fprintf(f, "  }\n");
}

/*
Recursively print the subPath.
*/
void print_HPT_subpath_dot(Path* n, FILE *f)
{
  if(n->left)
  {
    print_HPT_ptnode(n, f);

    fprintf(f, "  ");
    print_HPT_node_dot(n, f);
    fprintf(f, " -> ");
    print_HPT_node_dot(n->left, f);
    fprintf(f, " [style=dashed];\n");

    print_HPT_subpath_dot(n->left, f);
  }
  if(n->right)
  {
    print_HPT_ptnode(n, f);

    fprintf(f, "  ");
    print_HPT_node_dot(n, f);
    fprintf(f, " -> ");
    print_HPT_node_dot(n->right, f);
    fprintf(f, " [style=dashed];\n");

    print_HPT_subpath_dot(n->right, f);
  }
  if(n->node)                         //node of alt_tree
  {
    print_HPT_hpnode(n, f);
    if(n->child_heavypath)
    {
      if(!n->child_heavypath->node)   //child is also not alt_tree node
      {
        fprintf(f, "  ");
        print_HPT_node_dot(n, f);
        fprintf(f, " -> ");
        print_HPT_node_dot(n->child_heavypath, f);
        fprintf(f, " [style=dashed];\n");
      }

      print_HPT_subpath_dot(n->child_heavypath, f);
    }
  }
}


/*
Recursively print the subtree.
*/
void print_HPT_subtree_dot(Node* node, FILE *f)
{
  if(node->nneigh > 1)    //not leaf
  {
    int firstchild = 1;
    if(node->nneigh == 2)
      firstchild = 0;

    for(int i = firstchild; i < node->nneigh; i++)
    {
      fprintf(f, "  ");
      print_HPT_node_dot(node->path, f);
      fprintf(f, " -> ");
      print_HPT_node_dot(node->neigh[i]->path, f);
      fprintf(f, ";\n");

      print_HPT_subtree_dot(node->neigh[i], f);
    }
  }
}


/*
Print a string representing this Path node formatted for dot output.
*/
void print_HPT_node_dot(Path* n, FILE *f)
{
  fprintf(f, "%i", n->id);
}

/*
Print a string that formats a heavypath (alt_tree) node in a PT.
*/
void print_HPT_hpnode(Path* n, FILE *f)
{
  if(n->node->nneigh == 1)              //a leaf of alt_tree
    fprintf(f, "  %i [label=\"%i: %s", n->id, n->node->id, n->node->name);
  else
    fprintf(f, "  %i [label=\"%i", n->id, n->node->id);
  fprintf(f, "\n%i %i %i\n%i %i %i", n->d_lazy, n->diff_path, n->diff_subtree,
          n->d_min, n->d_min_path, n->d_min_subtree);
  fprintf(f, "\"];\n");
}

/* Print a string that formats a PT node.
*/
void print_HPT_ptnode(Path* n, FILE *f)
{
  fprintf(f, "  %i [shape=rectangle ", n->id);
  fprintf(f, "label=\"%i", n->id);
  fprintf(f, "\n%i %i %i\n%i %i %i", n->d_lazy, n->diff_path, n->diff_subtree,
          n->d_min, n->d_min_path, n->d_min_subtree);
  fprintf(f, "\"];\n");
}