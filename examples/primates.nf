params.align="DNA_primates.phy"
params.resdir="results"

resdir=file(params.resdir)
resdir.with{mkdirs()}

align=file(params.align)

process InferTree {
	input:
	file align

	output:
	set file("ref.nw"),file("boot.nw") into trees

	shell:
	'''
	#/usr/bin/env bash
	phyml -i DNA_primates.phy -b 1000 -m GTR -v e -a e -t e -o tlr -d nt --quiet
	mv !{align}_phyml_tree.txt ref.nw
	mv !{align}_phyml_boot_trees.txt boot.nw
	'''
}

trees.subscribe{
	r,b -> r.copyTo(resdir.resolve(r.name));b.copyTo(resdir.resolve(b.name))   
}
