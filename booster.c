#include "io.h"
#include "tree.h"

#include <string.h> /* for strcpy, strdup, etc */
#include <libgen.h> /* for basename & dirname */
#include <dirent.h> /* to read directories, including with the scandir function */
#include <getopt.h>
#include <omp.h> /* OpenMP */

#include "version.h"

static char* filename_prefix;		/* static global var only used to pass pseudo-argument to the following function */
int filename_filter(const struct dirent *file_to_test) {
  return !strncmp(file_to_test->d_name, filename_prefix, strlen(filename_prefix));
  /* tests that filename_prefix is a prefix of the filename */
} /* end of filename_filter, who returns 0 iff the two strings are different */

void usage(FILE * out,char *name){
  fprintf(out,"Usage: ");
  fprintf(out,"%s -i <tree file> -b <bootstrap prefix or file> [-r <# rand shufling> -n <normalization> -@ <cpus> -s <seed> -S <stat file> -o <output tree> -v]\n",name);
  fprintf(out,"Options:\n");
  fprintf(out,"      -i : Input tree file\n");
  fprintf(out,"      -b : Bootstrap prefix (e.g. boot_) or file containing several bootstrap trees\n");
  fprintf(out,"      -o : Output file (optional), default : stdout\n");
  fprintf(out,"      -@ : Number of threads (default 1)\n");
  fprintf(out,"      -s : Seed (optional)\n");
  fprintf(out,"      -S : Prints output statistics for each branch in the given output file\n");
  fprintf(out,"      -r : Number of random shuffling (for empirical norm only). Default: 10\n");
  fprintf(out,"      -n : Sets the normalization strategy to \"auto\" (default), \"empirical\" or \"theoretical\"\n");
  fprintf(out,"          - empirical      : Normalizes the support by the expected mast distance computed\n");
  fprintf(out,"                             using random trees (shuffled from the reference tree)\n");
  fprintf(out,"          - theoretical    : Normalizes the support by the expected mast distance computed as\n");
  fprintf(out,"                             p-1, p=the number of taxa on the lightest side of the bipartition\n");
  fprintf(out,"          - auto (default) : Will choose automatically : empirical if < 1000 taxa, theoretical\n");
  fprintf(out,"                             otherwise\n");
  fprintf(out,"      -v : Prints version (optional)\n");
  fprintf(out,"      -h : Prints this help\n");
}

void printOptions(FILE * out,char* input_tree,char * boot_trees, char * output_tree, char *output_stat, char *norm_strategy, long seed, int nb_threads, int nb_rand_shuf){
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
  fprintf(out,"Normalization   : %s\n", norm_strategy);
  fprintf(out,"Nb Rand Shuf    : %d\n", nb_rand_shuf);
  fprintf(out,"Seed            : %ld\n", seed);
  fprintf(out,"Threads         : %d\n", nb_threads);
  fprintf(out,"**************************\n");
}

void reset_matrices(int nb_taxa, int nb_edges_ref, int nb_edges_boot, short unsigned*** c_matrix, short unsigned*** i_matrix, short unsigned*** hamming, short unsigned** min_dist){
  int i;
  (*min_dist) = (short unsigned*) malloc(nb_edges_ref*sizeof(short unsigned)); /* array of min Hamming distances */
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

void free_matrices(int nb_edges_ref, short unsigned*** c_matrix, short unsigned*** i_matrix, short unsigned*** hamming, short unsigned** min_dist){
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
}

int main (int argc, char* argv[]) {
  /* this program takes as input three arguments.
     Arg1 is the filename of the reference tree.
     Arg2 is the prefix (including path if necessary) of the trees to be compared to the reference (bootstrapped trees)
     OR Arg2 is a single file containing all the bootstrap trees, one per line.
     Arg3 is the name of the output file (output tree with bootstrap values). */

  int i, retcode;
  /* int one_side; /\* to store a number of taxa seen on one side of a branch in the ref tree *\/ */

  /* force the buffering on stdout to a line buffering, for qsub not to annoy us and buffer it excessively */
  // setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
  long seed = -1;

  FILE *output_file = NULL;
  FILE *intree_file = NULL;
  FILE *boottree_file = NULL;
  FILE *stat_file = NULL;

  char *input_tree = NULL;
  char *boot_trees = NULL;
  char *out_tree = NULL;
  char *stat_out = NULL;

  char *norm_strategy = "auto";
  /* true if normalization strategy is done by computing (equation) and not 
     generating random trees*/
  int norm_theoretical = 0;
  
  Tree *ref_tree;
  Tree *shuf_tree;
  char **alt_tree_strings;
  Tree *alt_tree;

  /* Number of random tax shuffle per bootstrap tree */
  int nb_random = 10;
  int shuf = 0;
  int num_threads = 1;
	
  static struct option long_options[] = {
    {"input", required_argument, 0, 'i'},
    {"boot" , required_argument, 0, 'b'},
    {"out"  , required_argument, 0, 'o'},
    {"seed" , required_argument, 0, 's'},
    {"stat-file"  , required_argument, 0, 'S'},
    {"num-threads", required_argument, 0,'@'},
    {"normalization", required_argument, 0, 'n'},
    {"num-rand-shuf", required_argument, 0, 'r'},
    {"help" , no_argument      , 0, 'h'},
    {"version", no_argument      , 0, 'v'},
    {0, 0, 0, 0}
  };

  opterr = 0;
  int option_index = 0;
  int c = 0;
  while ((c = getopt_long(argc, argv, "i:b:o:s:@:S:n:r:hv", long_options, &option_index)) != -1){
    switch (c){
    case 'i': input_tree = optarg; break;
    case 'b': boot_trees = optarg; break;
    case 'o': out_tree = optarg; break;
    case '@': num_threads=strtol(optarg,NULL,10); break; 
    case 's': seed = strtol(optarg,NULL,10); break;
    case 'S': stat_out = optarg; break;
    case 'r': nb_random = strtol(optarg,NULL,10); break;
    case 'n': norm_strategy = optarg; break;
    case 'h': usage(stdout,argv[0]); return EXIT_SUCCESS; break; 
    case 'v': version(stdout,argv[0]); return EXIT_SUCCESS; break; 
    case ':': fprintf(stderr, "Option -%c requires an argument\n", optopt); return EXIT_FAILURE; break;
    case '?': fprintf(stderr, "Option -%c is undefined\n", optopt); return EXIT_FAILURE; break;
    }
  }

  if(seed!=-1){
    prng_seed_bytes (&seed, sizeof(seed));
  }else{
    seed = prng_seed_time();
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

  if(norm_strategy==NULL ||
     (strcmp(norm_strategy,"empirical") &&
      strcmp(norm_strategy,"theoretical") &&
      strcmp(norm_strategy,"auto"))){
    fprintf(stderr,"Normalization strategy must be one of \"empirical\", \"theoretical\" or \"auto\"\n");
    Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
  }else{
    if(!strcmp(norm_strategy,"theoretical"))
      norm_theoretical = 1;
  }
  
  printOptions(stderr, input_tree, boot_trees, out_tree, stat_out, norm_strategy, seed, num_threads, nb_random);

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
  //DEBUG printf("This is the tree string I am going to analyse: \"%s\"\n", big_string);

  /* and then feed this string to the parser */
  char** taxname_lookup_table = NULL;
  ref_tree  = complete_parse_nh(big_string, &taxname_lookup_table); /* sets taxname_lookup_table en passant */
  shuf_tree = complete_parse_nh(big_string, &taxname_lookup_table); /* sets taxname_lookup_table en passant */

  if(!strcmp(norm_strategy,"auto") && ref_tree->nb_taxa>1000){
    norm_theoretical = 1;
  }

  /*****************************************************************************/
  /* Preparing the bootstrap: arrays of counts and Hamming distances.          */
  /*****************************************************************************/

  /** Max number of branches we can see in the bootstrap tree:
      If it has no multifurcation : binary tree--> ntax*2-2 (if rooted...)
  */
  int max_branches_boot = ref_tree->nb_taxa*2-2;

  int n = ref_tree->nb_taxa;
  int m = ref_tree->nb_edges;


  short unsigned** c_matrix;
  short unsigned** i_matrix;
  short unsigned** hamming;
  short unsigned* min_dist;

  int *dist_accu      = (int*) calloc(m,sizeof(int)); /* array of distance sums, one per branch. Initialized to 0. */
  int **dist_accu_tmp;
  int **dist_accu_rand;
  int *dist_accu_rand_sum;

  /* For p.value computation */
  int **nb_gt_rand;
  int *nb_gt_rand_sum;

  /* initializations of the above: see inside loop */

  /***********************************************************************/
  /* Establishing the list of bootstrapped trees we are going to analyze */
  /***********************************************************************/

  char *dirtemp, *preftemp; /* just so that we can free this pointer at the end */
  char* dir_name = dirname(dirtemp = strdup(boot_trees));
  filename_prefix = basename(preftemp = strdup(boot_trees));
  struct dirent **matching_files;

  int num_bootstrap_files = scandir(dir_name, &matching_files, filename_filter, alphasort);
  /* printf("\nGoing to analyse %d files containing bootstrap trees.\n", num_bootstrap_files); */

  int init_boot_trees = 10;
  int i_tree;
  alt_tree_strings = malloc(init_boot_trees * sizeof(char*));


  int str_offset = strlen(dir_name);
  char tree_filename[str_offset + strlen(matching_files[0]->d_name) + 30];
  /* allowing 2 chars for the "/" separator and for the trailing "\0", plus some room if filename sizes vary across the set */
  strncpy(tree_filename, dir_name, str_offset);
  strncpy(tree_filename+(str_offset++),"/", 1);
	
  int file_index, num_trees = 0; /* this is the number of trees really analyzed */
	
  for (file_index = 0; file_index < num_bootstrap_files; file_index++) {
    /* printf("\n*** Processing file %s ***\n", matching_files[file_index]->d_name); */
    strcpy(tree_filename+str_offset, matching_files[file_index]->d_name); /* includes the null termination */
	  
    boottree_file = fopen(tree_filename,"r");
    if (boottree_file == NULL) {
      fprintf(stderr,"File %s not found or impossible to access media. Aborting.\n", tree_filename);
      Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
    }

    if (tell_size_of_one_tree(tree_filename) > treefilesize /* this value is still reachable */) {
      fprintf(stderr,"error: size of one alternate tree bigger than three times the size of the ref tree! Aborting.\n");
      Generic_Exit(__FILE__,__LINE__,__FUNCTION__,EXIT_FAILURE);
    }

    /* we copy the tree into a large string */
    while(copy_nh_stream_into_str(boottree_file, big_string)) /* reads from the current point in the stream, retcode 1 iff no error */
      {
	/* fprintf(stderr,"\n*** Processing one boot tree  ***\n"); */
	if(num_trees >= init_boot_trees){
	  alt_tree_strings = realloc(alt_tree_strings,init_boot_trees*2*sizeof(char*));
	  init_boot_trees *= 2;
	}
	alt_tree_strings[num_trees] = strdup(big_string);
	num_trees++;
      }
    fclose(boottree_file);
  } /* end of the loop on all the bootstrap files */

  fprintf(stderr,"Num trees: %d\n",num_trees);
  
  dist_accu_tmp      = (int**) calloc(num_trees,sizeof(int*)); /* array of distance sums, one per boot tree and branch. Initialized to 0. */
  dist_accu_rand     = (int**) calloc(num_trees,sizeof(int*)); /* array of distance sums, one per branch. Initialized to 0. */
  nb_gt_rand         = (int**) calloc(num_trees,sizeof(int*)); /* array of distance sums, one per branch. Initialized to 0. */
  nb_gt_rand_sum     = (int*) calloc(m,sizeof(int)); /* array of distance sums, one per branch. Initialized to 0. */
  dist_accu_rand_sum = (int*) calloc(m,sizeof(int)); /* array of distance sums, one per branch. Initialized to 0. */
  for(i_tree=0; i_tree< num_trees; i_tree++){
    dist_accu_tmp[i_tree]  = (int*) calloc(m,sizeof(int)); /* array of distance sums, one per branch. Initialized to 0. */
    dist_accu_rand[i_tree]  = (int*) calloc(m,sizeof(int)); /* array of distance sums, one per branch. Initialized to 0. */
    nb_gt_rand[i_tree] = (int*) calloc(m,sizeof(int)); /* array of distance sums, one per branch. Initialized to 0. */
  }

  #pragma omp parallel for firstprivate(seed) private(min_dist,c_matrix,i_matrix,hamming,i,alt_tree) shared(max_branches_boot, ref_tree, alt_tree_strings, dist_accu_tmp, taxname_lookup_table, m) schedule(dynamic)
  for(i_tree=0; i_tree< num_trees; i_tree++){
    fprintf(stderr,"New bootstrap tree\n");
    seed ^= (omp_get_thread_num() + i_tree);
    prng_seed_bytes (&seed, sizeof(seed));
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
    reset_matrices(n, m, max_branches_boot, &c_matrix, &i_matrix, &hamming, &min_dist);

    /****************************************************/
    /* comparison of the bipartitions, MAST-like method */
    /****************************************************/		  
    /* calculation of the C and I matrices (see Brehelin/Gascuel/Martin) */
    update_all_i_c_post_order_ref_tree(ref_tree, alt_tree, i_matrix, c_matrix);
    update_all_i_c_post_order_boot_tree(ref_tree, alt_tree, i_matrix, c_matrix, hamming, min_dist);
    /* output, just to see */
    /* fprintf(stderr,"nb taxa: %d, nb edges: %d\n", n,m); */
    for (i = 0; i < m; i++) {
      /* Just backup for pvalue computation */
      dist_accu_tmp[i_tree][i] = min_dist[i];
    }
    free_matrices(m, &c_matrix, &i_matrix, &hamming, &min_dist);
    free_tree(alt_tree);
  }

  #pragma omp barrier


  for (i = 0; i < m; i++){
    for(i_tree=0; i_tree < num_trees; i_tree++){
      dist_accu[i] += dist_accu_tmp[i_tree][i];
    }
  }

  for(shuf = 0; !norm_theoretical && shuf < nb_random ; shuf++){
    #pragma omp barrier
    fprintf(stderr,"Shuffle n°%d\n", shuf);
    shuffle_taxa(shuf_tree);
    #pragma omp parallel for private(min_dist,c_matrix,i_matrix,hamming,i,alt_tree) shared(max_branches_boot, shuf,shuf_tree, alt_tree_strings, nb_gt_rand, dist_accu_rand, dist_accu_tmp, taxname_lookup_table, m) schedule(dynamic)
    for(i_tree=0; i_tree< num_trees; i_tree++){
      alt_tree = complete_parse_nh(alt_tree_strings[i_tree], &taxname_lookup_table);

      if (alt_tree == NULL) {
	fprintf(stderr,"Not a correct NH tree (%d). Skipping.\n%s\n",i_tree,alt_tree_strings[i_tree]);
	continue; /* some files maybe not containing trees */
      }
      if (alt_tree->nb_taxa != shuf_tree->nb_taxa) {
	fprintf(stderr,"This tree doesn't have the same number of taxa as the reference tree. Skipping.\n");
	continue; /* some files maybe not containing trees */
      }
      reset_matrices(n, m, max_branches_boot, &c_matrix, &i_matrix, &hamming, &min_dist);

      /* We do the same thing with nb_rand taxa shuffled original trees */
      for (i = 0; i < m; i++) {
	min_dist[i] = n; /* initialization to the nb of taxa */
      }
      update_all_i_c_post_order_ref_tree(shuf_tree, alt_tree, i_matrix, c_matrix);
      update_all_i_c_post_order_boot_tree(shuf_tree, alt_tree, i_matrix, c_matrix, hamming, min_dist);
      for (i = 0; i < m; i++) {
	if(dist_accu_tmp[i_tree][i] >= min_dist[i])
	  nb_gt_rand[i_tree][i]++;
	dist_accu_rand[i_tree][i] += min_dist[i];
      }
      free_matrices(m, &c_matrix, &i_matrix, &hamming, &min_dist);
      free_tree(alt_tree);
    }
  } /* end of the while loop on the retcode of read_from_string not being 0 */

  /* After all parallel computations, we aggregate all data */
  for(i_tree=0; i_tree < num_trees; i_tree++){
    for (i = 0; i < m; i++){
      if(norm_theoretical){
	dist_accu_rand_sum[i] += nb_random * (ref_tree->a_edges[i]->topo_depth-1);
      }else{
	nb_gt_rand_sum[i] += nb_gt_rand[i_tree][i];
	dist_accu_rand_sum[i] += dist_accu_rand[i_tree][i];
      }
    }
  }

  int card;
  double bootstrap_val, avg_dist, avg_rand_dist, pvalue;
		
  if(num_trees != 0) {
    if(stat_file != NULL)
      fprintf(stat_file,"EdgeId\tDepth\tMeanMinDist\tMeanMinDistRand\tPValue\n");

    /* OUTPUT FINAL STATISTICS and UPDATE REF TREE WITH BOOTSTRAP VALUES */
    /* printf("\n*******\nFINAL STATS\n*******\n"); */
    /* printf("%d bootstrap trees successfully analysed\n", num_trees); */
    for (i = 0; i <  ref_tree->nb_edges; i++) {
      /* printf("Edge#%d: ", i); */
      if(ref_tree->a_edges[i]->right->nneigh == 1) { continue; }
      /*if(ref_tree->a_edges[i]->had_zero_length) { continue; }*/

      /* the bootstrap value for a branch is inscribed as the name of its descendant (always right side of the edge, by convention) */
      if(ref_tree->a_edges[i]->right->name) free(ref_tree->a_edges[i]->right->name); /* clear name if existing */
      ref_tree->a_edges[i]->right->name = (char*) malloc(16 * sizeof(char));
      card = ref_tree->a_edges[i]->hashtbl[1]->num_items;
      if (card > n/2) { card = n - card; }	  

      avg_dist      = (double) dist_accu[i] * 1.0 / num_trees;
      avg_rand_dist = (double) dist_accu_rand_sum[i] * 1.0 / (num_trees * nb_random);
      if(norm_theoretical)
	pvalue = 1;
      else
	pvalue        = (double) nb_gt_rand_sum[i] * 1.0 / (num_trees * nb_random);
      bootstrap_val = (double) 1.0 - avg_dist * 1.0 / avg_rand_dist;

      if(stat_file != NULL)
	fprintf(stat_file,"%d\t%d\t%f\t%f\t%f\n", i, (ref_tree->a_edges[i]->topo_depth), avg_dist, avg_rand_dist, pvalue);

      sprintf(ref_tree->a_edges[i]->right->name, "%.6f", bootstrap_val);

      ref_tree->a_edges[i]->branch_support = bootstrap_val;
      /* printf("mast-like stability: %s\n", ref_tree->a_edges[i]->right->name); */
    } /* end for */

    write_nh_tree(ref_tree, output_file);
  }

  fclose(output_file);
  if(stat_file != NULL) fclose(stat_file);
  // FREEING STUFF
  free(dirtemp);
  free(preftemp);
  free(big_string);
  /* free the stuff left behind by scandir */
  for(i=0; i < num_bootstrap_files; i++) free(matching_files[i]); /* freeing (struct dirent*)'s */
  free(matching_files);

  /* free the stuff for the calculation of the mast-like distances */
  free(dist_accu);
  for(i_tree=0; i_tree < num_trees;i_tree++){
    free(dist_accu_tmp[i_tree]);
    free(alt_tree_strings[i_tree]);
    free(dist_accu_rand[i_tree]);
    free(nb_gt_rand[i_tree]);
  }
  free(dist_accu_rand);
  free(dist_accu_rand_sum);
  free(alt_tree_strings);
  free(dist_accu_tmp);
  free(nb_gt_rand);
  free(nb_gt_rand_sum);

  /* we also have to free the taxname lookup table */
  for(i=0; i < ref_tree->nb_taxa; i++) free(taxname_lookup_table[i]); /* freeing (char*)'s */
  free(taxname_lookup_table); /* which is a (char**) */
  free_tree(ref_tree);
  return 0;
}
