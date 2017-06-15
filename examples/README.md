#  Example dataset

You may try _booster_ with the following trees inferred from primate nt alignment
taken from ["The Phylogenetic Handbook"](http://www.cambridge.org/catalogue/catalogue.asp?isbn=9780521877107).

* Reference tree: ref.nw.gz
* 1000 Bootstrap trees: boot.nw.gz

If you want to also infer reference and bootstrap trees prior to using _booster_:

* Original alignment: DNA_primates.phy
* Nextflow workflow: primates.nf (`nextflow run primates.nf` to run it)

# Running the example

To run the example and compute TBE and FBP supports, you may follow the steps above:

```bash
gunzip ref.nw.gz
gunzip boot.nw.gz
booster -a tbe -i ref.nw -b boot.nw -i TBE.nw
booster -a fbp -i ref.nw -b boot.nw -i FBP.nw
```

# Results

After computing TBE and FBP using _booster_, you should obtain trees like the followings:

* TBE supports: TBE.nw
* FBP supports: FBP.nw
