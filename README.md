# BOOSTER : BOOtstrap Support by TransfER
BOOSTER is a new way of computing bootstrap supports in large phylogenies.

See [help page](https://booster.pasteur.fr/help) for more informations.

# Help

## Install BOOSTER

### Binaries
You can download ready to run binaries for the latest release in the [release](https://github.com/evolbioinfo/booster/releases) section.
Binaries are available for MacOS, Linux, and Windows.

### Docker
BOOSTER Docker image is accessible from [docker hub](https://hub.docker.com/r/evolbioinfo/booster/). You may use it as follows:

```[bash]
# Display booster help
docker run -v $PWD:$PWD -w $PWD -i -t evolbioinfo/booster -h
```

### Singularity
BOOSTER [docker image](https://hub.docker.com/r/evolbioinfo/booster/) is usable from singularity . You may use it as follows:

```[bash]
# Pull image from docker hub
singularity pull docker://evolbioinfo/booster
# Display booster help
./booster.simg -h
```
### From Sources
If previous installation methods do not fit your needs, you can build BOOSTER from sources.

BOOSTER depends on [OpenMP](https://fr.wikipedia.org/wiki/OpenMP), which should be installed first before building BOOSTER.

For example:
- On Ubuntu / Debian:
```
sudo apt-get install libgomp1
```
- On CentOS / RedHat:
```
sudo yum install libgomp
```

Then: 

* First download a [release](https://github.com/fredericlemoine/booster/releases) or clone the repository;
* enter the `src` directory and type `make`;
* booster executable should be located in the current directory.

## Usage

```
Usage: ./booster -i <ref tree file (newick)> -b <bootstrap tree file (newick)> [-d <dist_cutoff> -r <raw distance output tree file> -@ <cpus>  -S <stat file> -o <output tree> -v]
Options:
      -i : Input tree file
      -b : Bootstrap tree file (1 file containing all bootstrap trees)
      -a, --algo  : bootstrap algorithm, rtbe (rapid transfer bootstrap) tbe (transfer bootstrap) or fbp (Felsenstein bootstrap) (default rtbe)
      -o : Output file (optional), default : stdout
      -r, --out-raw : Output file (only with tbe, optional) with raw transfer distance as support values in the form of
                       id|avgdist|depth, default : none
      -@ : Number of threads (default 1)
      -S : Prints output logs in the given output file (average raw min transfer distance per branches, and average
      	   transfer index per taxa)
      -c, --count-per-branch : Prints individual taxa moves for each branches in the log file (only with -S and -a tbe)
      -d, --dist-cutoff: Distance cutoff to consider a branch for moving taxa computation (tbe only, default 0.3)
      -q, --quiet : Does not print progress messages during analysis
      -v : Prints version (optional)
      -h : Prints this help
```

## Options
* `-i`: Reference tree file : a reference tree in newick format;
* `-b`: Bootstrap tree file : a set of bootstrap trees in newick format;
* `-@`: Number of threads;
* `-a`: Bootstrap algorithm: `rtbe` (rapid Transfer Boostrap Expectation) `tbe` (Transfer Bootstrap Expectation) or `fbp` (Felsenstein Bootstrap Proportion);
* `-S`: Output statistic file;
* `-r`: If you need to analyze individual average transfer distances of branches computed during a TBE run (`-a tbe`), you can give this option `-r`. In that case, booster will output a tree in newick format in the given file, and that will contain average transfer distances as branch support, in the form `id|avgdist|depth`;
* `-c`: If you want to characterize the taxa responsible for a given tbe support, for example if you want to known wether a support of 70% is always due the same 30% species that move in all the bootstrap trees or not, you may use this option. It will print a matrix with branch ids in row, taxa in column, and each value is the percentage of bootstrap trees for which: 1) a minimum distance branch closest than the given cutoff (`-d`) exists; and 2) the taxon moves around that branch. Please note that with very large trees, the matrix may be very large as there is one row per internal branch, and one column per taxon. Finally, branch identifiers are given in the branch labels of the "raw distance tree" with option `-r`.

## Example of workflow

You have a nucleotide alignment and you want to compute booster supports with 100 bootstrap samples. The first step is to generate reference and bootstrap trees. Several ways to do it depending on the phylogenetic tool you want to use:

* PhyML: Standard bootstrap + TBE
```bash
# Compute trees
phyml -i align.phy -d nt -b 100 -m GTR -f e -t e -c 6 -a e -s SPR -o tlr 
# Compute booster supports
booster -a tbe -i align.phy_phyml_tree.txt -b align.phy_phyml_boot_trees.txt -@ 5 -o booster.nw
```

* PhyML only (beta): Need to download and build PhyML from its [github repository](https://github.com/stephaneguindon/phyml/)
```bash
# Compute trees + TBE supports
phyml -i align.phy -d nt -b 100 --tbe -m GTR -f e -t e -c 6 -a e -s SPR -o tlr 
```

* RAxML: Standard bootstrap + TBE
```bash
# Infer reference tree
raxmlHPC -m GTRGAMMA -p $RANDOM -s align.phy -n REF
# Infer bootstrap trees
raxmlHPC -m GTRGAMMA -p $RANDOM -b $RANDOM -# 100 -s align.phy -n BOOT
# Compute booster support
booster -a tbe -i RAxML_bestTree.REF -b RAxML_bootstrap.BOOT -@ 5 -o booster.nw
```

* RAxML: Rapid bootstrap + TBE
```bash
# Infer reference tree + bootstrap trees
raxmlHPC -f a -m GTRGAMMA -c 4 -s align.phy -n align -T 4 -p $RANDOM -x $RANDOM -# 100
# Compute booster support
booster -a tbe -i RAxML_bestTree.align -b RAxML_bootstrap.align -@ 5 -o booster.nw
```

* FastTree: You will need to generate bootstrap alignments (Phylip format), with [goalign](https://github.com/fredericlemoine/goalign) for example
```bash
# Build bootstrap alignments
goalign build seqboot -i align.phy -p -n 100 -o boot -S
# Infer reference tree
FastTree -nt -gtr align.phy > ref.nhx
# Infer bootstrap trees
cat boot*.ph | FastTree -nt -n 100 -gtr > boot.nhx
# Compute booster supports
booster -a tbe -i ref.nhx -b boot.nhx -@ 5 -o booster.nw
```

* IQ-TREE : Standard bootstrap + TBE
```
# Infer ML tree + standard bootstrap trees
iqtree-omp -s align.phy -m GTR -b 100 -nt 5
# Compute booster supports
booster -a tbe -i align.phy.treefile -b align.phy.boottrees -@ 5 -o booster.nw
```

# Reference

If you use BOOSTER, please cite this [article](https://www.nature.com/articles/s41586-018-0043-0):

```
Renewing Felsenstein's Phylogenetic Bootstrap in the Era of Big Data
F. Lemoine, J.-B. Domelevo-Entfellner, E. Wilkinson, D. Correia, M. Davila Felipe, T. De Oliveira, O. Gascuel.
Nature 556, 452-456 (2018)
```
