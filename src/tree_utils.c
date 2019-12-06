/*

BOOSTER: BOOtstrap Support by TransfER: 
BOOSTER is an alternative method to compute bootstrap branch supports 
in large trees. It uses transfer distance between bipartitions, instead
of perfect match.

Copyright (C) 2017 Frederic Lemoine, Jean-Baka Domelevo Entfellner, Olivier Gascuel

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include "tree_utils.h"

/**
   Generates a random tree based on the taxa of the tree and its lookuptable in argument 
   - Uses the same taxnames
   - Uses the taxname_lookup_id
   Advice: do a srand(time(NULL)) before calling this function a large number of times
*/
Tree* gen_random_tree(Tree *tree){
  int* indices = (int*) calloc(tree->nb_taxa, sizeof(int)); /* the array that we are going to shuffle around to get random order in the taxa names */
  int taxon;
  for(taxon = 0; taxon < tree->nb_taxa; taxon++) indices[taxon] = taxon; /* initialization */
  
  /* zero the number of taxa inserted so far in this tree */
  int nb_inserted_taxa = 0,edge_ind;
  Tree* my_tree = NULL;
  int i;
  /* shuffle the indices we are going to use to determine the names of leaves */
  shuffle(indices, tree->nb_taxa, sizeof(int));
  
  /* free the previous tree if existing */
  if(my_tree) free_tree(my_tree); 
  
  /* create a new tree */
  my_tree = new_tree(tree->taxa_names[indices[nb_inserted_taxa++]]);
	
  /* graft the second taxon */
  graft_new_node_on_branch(NULL, my_tree, 0.5, 1.0, tree->taxa_names[indices[nb_inserted_taxa++]]);
  while(nb_inserted_taxa < tree->nb_taxa) {
    /* select a branch at random */
    edge_ind = rand_to(my_tree->nb_edges); /* outputs something between 0 and (nb_edges-1) exclusive */
    graft_new_node_on_branch(my_tree->a_edges[edge_ind], my_tree, 0.5, 1.0, tree->taxa_names[indices[nb_inserted_taxa++]]);
  } /* end looping on the taxa, tree is full */

  /* here we need to re-root the tree on a trifurcated node, not on a leaf, before we write it in NH format */
  reroot_acceptable(my_tree);

  my_tree->taxname_lookup_table = tree->taxname_lookup_table;
  my_tree->nb_taxa = tree->nb_taxa;

  int e;
  for(e=0;e<my_tree->nb_edges;e++){
    my_tree->a_edges[e]->hashtbl = create_id_hash_table(my_tree->nb_taxa);
  }
  /* write_nh_tree(my_tree,stdout); */

  update_hashtables_post_alltree(my_tree);
  update_node_heights_post_alltree(my_tree);
  update_node_heights_pre_alltree(my_tree);

  /* topological depths of branches */
  update_all_topo_depths_from_hashtables(my_tree);
  free(indices);

  my_tree->leaves = allocateLA(my_tree->nb_taxa);
  
  prepare_rapid_TI(my_tree);
  return(my_tree);
}


Tree * gen_rand_tree(int nbr_taxa, char **taxa_names){
  int taxon;
  Tree *my_tree;
  int* indices = (int*) calloc(nbr_taxa, sizeof(int)); /* the array that we are going to shuffle around to get random order in the taxa names */
  /* zero the number of taxa inserted so far in this tree */
  int nb_inserted_taxa = 0;

  int i_edge, edge_ind;
  
  for(taxon = 0; taxon < nbr_taxa; taxon++)
    indices[taxon] = taxon;

  shuffle(indices, nbr_taxa, sizeof(int));
  
  if(taxa_names == NULL){
    taxa_names = (char**) calloc(nbr_taxa, sizeof(char*));
    for(taxon = 0; taxon < nbr_taxa; taxon++) {
      taxa_names[taxon] = (char*) calloc((int)(log10(nbr_taxa)+2), sizeof(char));
      sprintf(taxa_names[taxon],"%d",taxon+1); /* names taxa by a mere integer, starting with "1" */
    }
  }
  
  /* create a new tree */
  my_tree = new_tree(taxa_names[indices[nb_inserted_taxa++]]);
  
  /* graft the second taxon */
  graft_new_node_on_branch(NULL, my_tree, 0.5, 1.0, taxa_names[indices[nb_inserted_taxa++]]);
  
  while(nb_inserted_taxa < nbr_taxa) {
    /* select a branch at random */
    edge_ind = rand_to(my_tree->nb_edges); /* outputs something between 0 and (nb_edges) exclusive */
    graft_new_node_on_branch(my_tree->a_edges[edge_ind], my_tree, 0.5, 1.0, taxa_names[indices[nb_inserted_taxa++]]);
  } /* end looping on the taxa, tree is full */
  
  /* here we need to re-root the tree on a trifurcated node, not on a leaf, before we write it in NH format */
  reroot_acceptable(my_tree);

  for(i_edge = 0; i_edge < my_tree->nb_edges; i_edge++){
    my_tree->a_edges[i_edge]->brlen = normal(0.1, 0.05);
    if(my_tree->a_edges[i_edge]->brlen < 0)
      my_tree->a_edges[i_edge]->brlen = 0;
    my_tree->a_edges[i_edge]->had_zero_length = (my_tree->a_edges[i_edge]->brlen < MIN_BRLEN);
  }

  my_tree->taxname_lookup_table = build_taxname_lookup_table(my_tree);
  
  for(i_edge=0;i_edge<my_tree->nb_edges;i_edge++){
    my_tree->a_edges[i_edge]->hashtbl = create_id_hash_table(my_tree->nb_taxa);
  }

  update_hashtables_post_alltree(my_tree);
  update_node_heights_post_alltree(my_tree);
  update_node_heights_pre_alltree(my_tree);

  /* topological depths of branches */
  update_all_topo_depths_from_hashtables(my_tree);
  my_tree->leaves = allocateLA(my_tree->nb_taxa);
  prepare_rapid_TI(my_tree);
  return(my_tree);
}
