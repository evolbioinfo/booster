# booster: BOOtstrap Support by Transfer
This tool is dedicated to compute bootstrap supports using transfer distance.

# Help
## Usage

```
Usage: booster -i <tree file> -b <bootstrap prefix or file> [-r <# rand shufling> -n <normalization> -@ <cpus> -s <seed> -S <stat file> -o <output tree> -v]
Options:
      -i : Input tree file
      -b : Bootstrap prefix (e.g. boot_) or file containing several bootstrap trees
      -o : Output file (optional), default : stdout
      -@ : Number of threads (default 1)
      -s : Seed (optional)
      -S : Prints output statistics for each branch in the given output file
      -r : Number of random shuffling (for empirical norm only). Default: 10
      -n : Sets the normalization strategy to "auto" (default), "empirical" or "theoretical"
          - empirical      : Normalizes the support by the expected mast distance computed
                             using random trees (shuffled from the reference tree)
          - theoretical    : Normalizes the support by the expected mast distance computed as
                             p-1, p=the number of taxa on the lightest side of the bipartition
          - auto (default) : Will choose automatically : empirical if < 1000 taxa, theoretical
                             otherwise
      -v : Prints version (optional)
      -h : Prints this help

```

## Options
* `-i`: Reference tree file : a reference tree in newick format
* `-b`: Bootstrap tree file : a set of bootstrap trees in newick format
* `-@`: Number of threads
* `-s`: Initial random seed
* `-S`: Output statistic file
* `-n`: Normalization strategy, empirical or theoretical
    * empirical: Shuffles n times (`-r` option) the reference tree to compute the distribution of the random transfer distance
    * theoretical: normalize transfer distance by the depth in the tree (number of taxa in the lightest side of the bi-partition)
    * auto: select theoretical if the tree contains more than 1000 taxa
