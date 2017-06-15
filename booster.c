#include "io.h"
#include "tree.h"
#include "bitset_index.h"

#include <string.h> /* for strcpy, strdup, etc */
#include <getopt.h>
#include <omp.h> /* OpenMP */
#include <math.h>

#include "version.h"

void tbe(Tree *ref_tree, char **alt_tree_strings,char** taxname_lookup_table, FILE *stat_file, int num_trees, int quiet, double dist_cutoff);
void fbp(Tree *ref_tree, char **alt_tree_strings,char** taxname_lookup_table, int num_trees, int quiet);

void usage(FILE * out,char *name){
  fprintf(out,"Usage: ");
  fprintf(out,"%s -i <tree file> -b <bootstrap prefix or file> [-@ <cpus>  -S <stat file> -o <output tree> -v]\n",name);
  fprintf(out,"Options:\n");
  fprintf(out,"      -i, --input      : Input tree file\n");
  fprintf(out,"      -b, --boot       : Bootstrap tree file (1 file containing all bootstrap trees)\n");
  fprintf(out,"      -o, --out        : Output file (optional), default : stdout\n");
  fprintf(out,"      -@, --num-threads: Number of threads (default 1)\n");
  fprintf(out,"      -S, --stat-file  : Prints output statistics for each branch in the given output file\n");
  fprintf(out,"      -d, --dist-cutoff: Distance cutoff to consider a branch for moving taxa computation (tbe only, default 0.3)\n");
  fprintf(out,"      -a, --algo       : tbe or fbp (default tbe\n");
  fprintf(out,"      -q, --quiet      : Does not print progress messages during analysis\n");
  fprintf(out,"      -v, --version    : Prints version (optional)\n");
  fprintf(out,"      -h, --help       : Prints this help\n");
}

void printOptions(FILE * out,char* input_tree,char * boot_trees, char * output_tree, char *output_stat, char *algo, int nb_threads, int quiet, int dist_cutoff){
  fprintf(out,"**************************\n");
  fprintf(out,"*         Options        *\n");
  fprintf(out,"**************************\n");
  short_version(out);
  fprintf(out,"Input Tree      : %s\n", input_tree);
  fprintf(out,"Bootstrap Trees : %s\n", boot_trees);
  if(output_tree==NULL)
    fprintf(out,"Output tree     : stdout\n");
  else
    fprintf(out,"Output tree     : %s\n",output_tree);
  if(output_stat==NULL)
    fprintf(out,"Stat file       : None\n");
  else
    fprintf(out,"Stat file       : %s\n",output_stat);
  fprintf(out,"Algo            : %s\n", algo);
  fprintf(out,"Threads         : %d\n", nb_threads);
  fprintf(out,"Dist cutoff     : %d\n", dist_cutoff);
  if(quiet)
    fprintf(out,"Quiet           : true\n");
  else
    fprintf(out,"Quiet           : false\n");
  fprintf(out,"**************************\n");
}

void reset_matrices(int nb_taxa, int nb_edges_ref, int nb_edges_boot, short unsigned*** c_matrix, short unsigned*** i_matrix, short unsigned*** hamming, short unsigned** min_dist, short unsigned** min_dist_edges){
  int i;
  (*min_dist) = (short unsigned*) malloc(nb_edges_ref*sizeof(short unsigned)); /* array of min Hamming distances */
  (*min_dist_edges) = (short unsigned*) malloc(nb_edges_ref*sizeof(short unsigned)); /* array of edge ids corresponding to min Hamming distances */
  (*c_matrix) = (short unsigned**) malloc(nb_edges_ref*sizeof(short unsigned*)); /* matrix of cardinals of complements */
  (*i_matrix) = (short unsigned**) malloc(nb_edges_ref*sizeof(short unsigned*)); /* matrix of cardinals of intersections */
  (*hamming) = (short unsigned**) malloc(nb_edges_ref*sizeof(short unsigned*)); /* matrix of Hamming distances */
  for (i=0; i<nb_edges_ref; i++){
    (*c_matrix)[i] = (short unsigned*) malloc(nb_edges_boot*sizeof(short unsigned));
    (*i_matrix)[i] = (short unsigned*) malloc(nb_edges_boot*sizeof(short unsigned));
    (*hamming)[i] = (short unsigned*) malloc(nb_edges_boot*sizeof(short unsigned));
    (*min_dist)[i] = nb_taxa; /* initialization to the nb of taxa */
  }
}

void free_matrices(int nb_edges_ref, short unsigned*** c_matrix, short unsigned*** i_matrix, short unsigned*** hamming, short unsigned** min_dist, short unsigned** min_dist_edges){
  int i;
  for (i=0; i<nb_edges_ref; i++) {
    free((*c_matrix)[i]);
    free((*i_matrix)[i]);
    free((*hamming)[i]);
  }
  free((*c_matrix));
  free((*i_matrix));
  free((*hamming));
  free((*min_dist));
  free((*min_dist_edges));
}


// Returns the list of id of species to move to go from one branch to the other
// Its length should correspond to given dist
// If not, exit with an error
int* species_to_move(Edge* re, Edge* be, int dist, int nb_taxa) {
  int i;
  int maxnb = dist;
  if(nb_taxa-dist >= dist) maxnb=nb_taxa-dist;
  int *diff = calloc(maxnb,sizeof(int));
  int *equ  = calloc(maxnb,sizeof(int));
  int nbdiff=0, nbequ=0;

  for(i = 0; i < nb_taxa; i++) {
    if(lookup_id(re->hashtbl[1],i) != lookup_id(be->hashtbl[1],i)){
      diff[nbdiff]=i;
      nbdiff++;
    } else {
      equ[nbequ] = i;
      nbequ++;
    }
  }
  if(nbdiff < nbequ){
    if(nbdiff != dist){
      fprintf(stderr,"Length of moved species array (%d) is not equal to the minimum distance found (%d)\n", nbdiff, dist);
      Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
    }
    free(equ);
    return diff;
  }
  if(nbequ != dist){
      fprintf(stderr,"Length of moved species array (%d) is not equal to the minimum distance found (%d)\n", nbequ, dist);
      Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
    }
  free(diff);
  return equ;
}

int main (int argc, char* argv[]) {
  /* this program takes as input three arguments.
     Arg1 is the filename of the reference tree.
     Arg2 is the prefix (including path if necessary) of the trees to be compared to the reference (bootstrapped trees)
     OR Arg2 is a single file containing all the bootstrap trees, one per line.
     Arg3 is the name of the output file (output tree with bootstrap values). */

  int i, retcode;
  /* int one_side; /\* to store a number of taxa seen on one side of a branch in the ref tree *\/ */

  FILE *output_file = NULL;
  FILE *intree_file = NULL;
  FILE *boottree_file = NULL;
  FILE *stat_file = NULL;

  char *input_tree = NULL;
  char *boot_trees = NULL;
  char *out_tree = NULL;
  char *stat_out = NULL;

  Tree *ref_tree;
  char **alt_tree_strings;

  char *algo = "tbe";
  
  int quiet = 0;
  
  int num_threads = 1;

  double dist_cutoff = 0.3;
	
  static struct option long_options[] = {
    {"input", required_argument, 0, 'i'},
    {"boot" , required_argument, 0, 'b'},
    {"out"  , required_argument, 0, 'o'},
    {"stat-file" , required_argument, 0, 'S'},
    {"algo" , required_argument, 0, 'a'},
    {"dist-cutoff" , required_argument, 0, 'd'},
    {"num-threads", required_argument, 0,'@'},
    {"help" , no_argument      , 0, 'h'},
    {"version", no_argument      , 0, 'v'},
    {"quiet", no_argument      , 0, 'q'},
    {0, 0, 0, 0}
  };

  opterr = 0;
  int option_index = 0;
  int c = 0;
  while ((c = getopt_long(argc, argv, "i:a:b:d:o:s:@:S:n:r:hvq", long_options, &option_index)) != -1){
    switch (c){
    case 'i': input_tree = optarg; break;
    case 'b': boot_trees = optarg; break;
    case 'o': out_tree = optarg; break;
    case '@': num_threads=strtol(optarg,NULL,10); break; 
    case 'a': algo = optarg; break;
    case 'd': sscanf(optarg,"%lf",&dist_cutoff); break;
    case 'S': stat_out = optarg; break;
    case 'q': quiet = 1; break;
    case 'h': usage(stdout,argv[0]); return EXIT_SUCCESS; break; 
    case 'v': version(stdout,argv[0]); return EXIT_SUCCESS; break;
    case ':': fprintf(stderr, "Option -%c requires an argument\n", optopt); return EXIT_FAILURE; break;
    case '?': fprintf(stderr, "Option -%c is undefined\n", optopt); return EXIT_FAILURE; break;
    }
  }

  if(strcmp(algo,"tbe") && strcmp(algo,"fbp")){
    fprintf(stderr,"Algo option must be one of \"tbe\" or \"fbp\"\n");
    Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
  }
  
  if (argc < optind || input_tree == NULL || boot_trees == NULL){
    fprintf(stderr,"An option is missing\n");
    usage(stderr,argv[0]);
    Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
  }

  if(num_threads>0){
    if(num_threads > omp_get_max_threads())
      num_threads = omp_get_max_threads();
  }else{
    num_threads = 1;
  }
  omp_set_num_threads(num_threads);

  if(stat_out !=NULL){
    stat_file = fopen(stat_out,"w");
    if(stat_file == NULL){
      fprintf(stderr,"File %s not found or not writable. Aborting.\n", stat_out);
      Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
    }
  } else stat_file = NULL;

  /* writing the output tree to the file given on the commandline */
  if(out_tree == NULL){
    output_file = stdout;
  }else{
    output_file = fopen(out_tree,"w");
    if(output_file == NULL){
      fprintf(stderr,"File %s not found or not writable. Aborting.\n", out_tree);
      Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
    }
  }

  if(!quiet) printOptions(stderr, input_tree, boot_trees, out_tree, stat_out, algo, num_threads, quiet, dist_cutoff);

  intree_file = fopen(input_tree,"r");
  if (intree_file == NULL) {
    fprintf(stderr,"File %s not found or impossible to access media. Aborting.\n", input_tree);
    Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
  }

  /* we copy the tree into a large string */
  unsigned int treefilesize = 3 * tell_size_of_one_tree(input_tree);
  if (treefilesize > MAX_TREELENGTH) {
    fprintf(stderr,"Tree filesize for %s bigger than %d bytes: are you sure it's a valid NH tree? Aborting.\n", input_tree, MAX_TREELENGTH/3);
    Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
  }

  char *big_string = (char*) calloc(treefilesize+1, sizeof(char)); 
  retcode = copy_nh_stream_into_str(intree_file, big_string);
  if (retcode != 1) { 
    fprintf(stderr,"Unexpected EOF while parsing the reference tree! Aborting.\n"); 
    Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
  }
  fclose(intree_file);

  /* and then feed this string to the parser */
  char** taxname_lookup_table = NULL;
  ref_tree  = complete_parse_nh(big_string, &taxname_lookup_table); /* sets taxname_lookup_table en passant */

  /***********************************************************************/
  /* Establishing the list of bootstrapped trees we are going to analyze */
  /***********************************************************************/
  int init_boot_trees = 10;
  int i_tree;
  int num_trees = 0; /* this is the number of trees really analyzed */

  alt_tree_strings = malloc(init_boot_trees * sizeof(char*));
  boottree_file = fopen(boot_trees,"r");
  if (boottree_file == NULL) {
    fprintf(stderr,"File %s not found or impossible to access media. Aborting.\n", boot_trees);
    Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
  }

  if (tell_size_of_one_tree(boot_trees) > treefilesize /* this value is still reachable */) {
    fprintf(stderr,"error: size of one alternate tree bigger than three times the size of the ref tree! Aborting.\n");
    Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
  }

  /* we copy the tree into a large string */
  while(copy_nh_stream_into_str(boottree_file, big_string)) /* reads from the current point in the stream, retcode 1 iff no error */
    {
      if(num_trees >= init_boot_trees){
	alt_tree_strings = realloc(alt_tree_strings,init_boot_trees*2*sizeof(char*));
	init_boot_trees *= 2;
      }
      alt_tree_strings[num_trees] = strdup(big_string);
      num_trees++;
    }
  fclose(boottree_file);

  if(!quiet)  fprintf(stderr,"Num trees: %d\n",num_trees);

  if(!strcmp(algo,"tbe")){
    tbe(ref_tree, alt_tree_strings, taxname_lookup_table, stat_file, num_trees, quiet, dist_cutoff);
  }else{
    fbp(ref_tree, alt_tree_strings, taxname_lookup_table, num_trees, quiet);
  }
  write_nh_tree(ref_tree, output_file);

  fclose(output_file);
  if(stat_file != NULL) fclose(stat_file);
  // FREEING STUFF
  free(big_string);

  /* free the stuff for the calculation of the mast-like distances */
  for(i_tree=0; i_tree < num_trees;i_tree++){
    free(alt_tree_strings[i_tree]);
  }
  free(alt_tree_strings);

  /* we also have to free the taxname lookup table */
  for(i=0; i < ref_tree->nb_taxa; i++) free(taxname_lookup_table[i]); /* freeing (char*)'s */
  free(taxname_lookup_table); /* which is a (char**) */
  free_tree(ref_tree);
  return 0;
}


void fbp(Tree *ref_tree, char **alt_tree_strings,char** taxname_lookup_table, int num_trees, int quiet){
  int j;
  Tree *alt_tree;
  int i_tree,i;
  short unsigned* nb_found = malloc(ref_tree->nb_edges * sizeof(short unsigned));
  double support;
  // We initialize the reference edge hashmap
  bitset_hashmap *hm = new_bitset_hashmap(ref_tree->nb_edges*2, 0.75);

  for(i=0; i< ref_tree->nb_edges; i++){
    nb_found[i] = 0;
    bitset_hashmap_putvalue(hm,ref_tree->a_edges[i]->hashtbl[1],ref_tree->nb_taxa,i);
  }

 
#pragma omp parallel for private( j, alt_tree, support) shared(nb_found, hm, ref_tree, alt_tree_strings, taxname_lookup_table, quiet, num_trees) schedule(dynamic)
  for(i_tree=0; i_tree< num_trees; i_tree++){
    if(!quiet) fprintf(stderr,"New bootstrap tree : %d\n",i_tree);
    alt_tree = complete_parse_nh(alt_tree_strings[i_tree], &taxname_lookup_table);
    
    if (alt_tree == NULL) {
      fprintf(stderr,"Not a correct NH tree (%d). Skipping.\n%s\n",i_tree,alt_tree_strings[i_tree]);
      continue; /* some files maybe not containing trees */
    }
    if (alt_tree->nb_taxa != ref_tree->nb_taxa) {
      fprintf(stderr,"This tree doesn't have the same number of taxa as the reference tree. Skipping.\n");
      continue; /* some files maybe not containing trees */
    }

    /****************************************************/
    /*     comparison of the bipartitions, FBP method   */
    /****************************************************/		  
    for (j = 0; j <  alt_tree->nb_edges; j++) {
      // We query the hashmap to see if the edge is present, and then get its reference index
      int refindex = bitset_hashmap_value(hm, alt_tree->a_edges[j]->hashtbl[1], alt_tree->nb_taxa);
      if (refindex>-1){
	#pragma omp atomic update
	nb_found[refindex]++;
      }
    }
    free_tree(alt_tree);
  }

  #pragma omp barrier

  if(num_trees != 0) {
    for (i = 0; i <  ref_tree->nb_edges; i++) {
      if(ref_tree->a_edges[i]->right->nneigh == 1) { continue; }
      /* the bootstrap value for a branch is inscribed as the name of its descendant (always right side of the edge, by convention) */
      if(ref_tree->a_edges[i]->right->name) free(ref_tree->a_edges[i]->right->name); /* clear name if existing */
      ref_tree->a_edges[i]->right->name = (char*) malloc(16 * sizeof(char));
      support   = (double) nb_found[i] * 1.0 / num_trees;
      sprintf(ref_tree->a_edges[i]->right->name, "%.6f", support);
      ref_tree->a_edges[i]->branch_support = support;
    }
  }
  free(nb_found);
  free_bitset_hashmap(hm);
}

void tbe(Tree *ref_tree, char **alt_tree_strings,char** taxname_lookup_table, FILE *stat_file, int num_trees, int quiet, double dist_cutoff){
  short unsigned** c_matrix;
  short unsigned** i_matrix;
  short unsigned** hamming;
  short unsigned* min_dist_edge; /* array of edge ids corresponding to min Hamming distances */
  short unsigned* min_dist;
  int i;
  int m = ref_tree->nb_edges;
  int n = ref_tree->nb_taxa;
  Tree *alt_tree;
  int i_tree;
  int *dist_accu      = (int*) calloc(m,sizeof(int)); /* array of distance sums, one per branch. Initialized to 0. */
  int **dist_accu_tmp;
  double *moved_species_counts;  /* array of average branch rate in which each taxon moves */
  int *moved_species; /* array of number of branches in which each taxon moves, in one bootstrap tree: initialized at each bootstrap tree */
  /** Max number of branches we can see in the bootstrap tree: If it has no multifurcation : binary tree--> ntax*2-2 (if rooted...) */
  int max_branches_boot = ref_tree->nb_taxa*2-2;
  
  dist_accu_tmp      = (int**) calloc(num_trees,sizeof(int*)); /* array of distance sums, one per boot tree and branch. Initialized to 0. */
  for(i_tree=0; i_tree< num_trees; i_tree++){
    dist_accu_tmp[i_tree]  = (int*) calloc(m,sizeof(int)); /* array of distance sums, one per branch. Initialized to 0. */
  }
  moved_species_counts = (double*) calloc(m,sizeof(double)); /* array of average branch rate in which each taxon moves */

#pragma omp parallel for private(min_dist,c_matrix,i_matrix,hamming,min_dist_edge, i, alt_tree, moved_species) shared(max_branches_boot, ref_tree, alt_tree_strings, dist_accu_tmp, taxname_lookup_table, m, moved_species_counts) schedule(dynamic)
  for(i_tree=0; i_tree< num_trees; i_tree++){
    if(!quiet) fprintf(stderr,"New bootstrap tree : %d\n",i_tree);
    alt_tree = complete_parse_nh(alt_tree_strings[i_tree], &taxname_lookup_table);
    
    if (alt_tree == NULL) {
      fprintf(stderr,"Not a correct NH tree (%d). Skipping.\n%s\n",i_tree,alt_tree_strings[i_tree]);
      continue; /* some files maybe not containing trees */
    }
    if (alt_tree->nb_taxa != ref_tree->nb_taxa) {
      fprintf(stderr,"This tree doesn't have the same number of taxa as the reference tree. Skipping.\n");
      continue; /* some files maybe not containing trees */
    }

    /* resetting the arrays that need be reset. By construction of the post-order traversal,
       the other arrays (i_matrix, c_matrix and hamming) need not be reset. */
    reset_matrices(n, m, max_branches_boot, &c_matrix, &i_matrix, &hamming, &min_dist,&min_dist_edge);

    /****************************************************/
    /* comparison of the bipartitions, Transfer method */
    /****************************************************/		  
    /* calculation of the C and I matrices (see Brehelin/Gascuel/Martin) */
    update_all_i_c_post_order_ref_tree(ref_tree, alt_tree, i_matrix, c_matrix);
    update_all_i_c_post_order_boot_tree(ref_tree, alt_tree, i_matrix, c_matrix, hamming, min_dist, min_dist_edge);

    /* Looking at number of times each taxon moves around low distance branches */
    moved_species = (int*) calloc(ref_tree->nb_taxa,sizeof(int));
    int nb_branches_close=0;
    int j;
    for(i=0;i<m;i++){
      Edge* re = ref_tree->a_edges[i];
      if (re->right->nneigh == 1) continue;
      Edge* be = alt_tree->a_edges[min_dist_edge[i]];

      double norm  = ((double)min_dist[i]) * 1.0 / (((double)re->topo_depth) - 1.0);
      int mindepth = (int)(ceil(1.0/dist_cutoff + 1.0));
      if (norm <= dist_cutoff && re->topo_depth >= mindepth ){
	int* sm = species_to_move(re, be, min_dist[i], ref_tree->nb_taxa);
	for (j=0;j<min_dist[i];j++){
	  moved_species[sm[j]]++;
	}
	nb_branches_close++;
	free(sm);
      }
    }

    /* output, just to see */
    for (i = 0; i < m; i++) {
      /* Just backup for pvalue computation */
      dist_accu_tmp[i_tree][i] = min_dist[i];
    }
    for (i=0; i < ref_tree->nb_taxa; i++){
      #pragma omp atomic update
      moved_species_counts[i] += ((double)moved_species[i])*1.0/((double)nb_branches_close);
    }

    free_matrices(m, &c_matrix, &i_matrix, &hamming, &min_dist,&min_dist_edge);
    free_tree(alt_tree);
    free(moved_species);
  }

  #pragma omp barrier

  for (i = 0; i < m; i++){
    for(i_tree=0; i_tree < num_trees; i_tree++){
      dist_accu[i] += dist_accu_tmp[i_tree][i];
    }
  }

  int card;
  double bootstrap_val, avg_dist;
		
  if(num_trees != 0) {
    if(stat_file != NULL)
      fprintf(stat_file,"EdgeId\tDepth\tMeanMinDist\n");

    /* OUTPUT FINAL STATISTICS and UPDATE REF TREE WITH BOOTSTRAP VALUES */
    for (i = 0; i <  ref_tree->nb_edges; i++) {
      if(ref_tree->a_edges[i]->right->nneigh == 1) { continue; }

      /* the bootstrap value for a branch is inscribed as the name of its descendant (always right side of the edge, by convention) */
      if(ref_tree->a_edges[i]->right->name) free(ref_tree->a_edges[i]->right->name); /* clear name if existing */
      ref_tree->a_edges[i]->right->name = (char*) malloc(16 * sizeof(char));
      card = ref_tree->a_edges[i]->hashtbl[1]->num_items;
      if (card > n/2) { card = n - card; }	  
      avg_dist      = (double) dist_accu[i] * 1.0 / num_trees;
      bootstrap_val = (double) 1.0 - avg_dist * 1.0 / (1.0 * ref_tree->a_edges[i]->topo_depth-1.0);

      if(stat_file != NULL)
	fprintf(stat_file,"%d\t%d\t%f\n", i, (ref_tree->a_edges[i]->topo_depth), avg_dist);

      sprintf(ref_tree->a_edges[i]->right->name, "%.6f", bootstrap_val);

      ref_tree->a_edges[i]->branch_support = bootstrap_val;
    }

    if(stat_file != NULL){
      fprintf(stat_file,"Taxa transfer indexes:\n");
      for(i=0; i<ref_tree->nb_taxa;i++){
	fprintf(stat_file,"%s : %f\n", taxname_lookup_table[i], moved_species_counts[i]*100.0 / ((double)num_trees));
      }
    }
  }
  free(dist_accu);
  for(i_tree=0; i_tree < num_trees;i_tree++){
    free(dist_accu_tmp[i_tree]);
  }
  free(dist_accu_tmp);
  free(moved_species_counts);
}
