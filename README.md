# APP-Miner: Detecting API Misuses via Automatically Mining API Path Patterns 

Nowadays, it is common practice for programmers to use APIs to realize specific functions without understanding the details of the internal working mechanism, which indicates the API patterns. However, violating API patterns will cause API misuses and can have profound security implications.

The tool, APP-Miner (API path pattern miner), a novel static analysis framework for extracting API path patterns via a frequent subgraph mining technique from the source code without pattern templates. Up to Jul 12th, 2023, we have used APP-Miner to find 160 new API misuses. Moreover, we gained 20 CVEs. More details can be found in the paper shown at the bottom.

## How to use APP-Miner

### Prepare LLVM bitcode files of OS kernels
```sh
	$ cd deadline/code/srcs/
	# Just for example, you can change to the target software you want
	$ git clone git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
```

* We have modified "deadline" tool to generate bitcode files. (https://github.com/sslab-gatech/deadline.git)

### Build the code preprocessor
```sh 
	$ cd ../../../preprocessor/analyzer/
	$ make 
	# Now, you can find the executable, `kanalyzer`, in `preprocessor/analyzer/build/lib/`
```

* We have modified of "Crix" tool to preprocess the bitcode files. (https://github.com/umnsec/crix.git)

### Run the APP-Miner
```sh
	$ cd ../../app-miner/
	# The "source" should be absolute path
	# The "configCmd" is the command to configure the software
	# The "buildCmd" is the command to build the software 
	$ python3 main.py --process=parseIR --source=/home/APP-Miner/deadline/code/srcs/linux --configCmd="make allyesconfig" --buildCmd="make"
```

### Analyze results

* The intermediate results and the final results will be stored in app-miner/result/linux ("linux" will be changed if other software is analyzed). The "xml" file stores the reported potential bugs. Each column from left to right represents: number, API path pattern location, API name, rank, API path pattern, support, API misuse location.

## More details
* [The APP-Miner paper (Security and Privacy'24)]
```sh
@INPROCEEDINGS {,
author = {J. Jiang and J. Wu and X. Ling and T. Luo and S. Qu and Y. Wu},
booktitle = {2024 IEEE Symposium on Security and Privacy (SP)},
title = {APP-Miner: Detecting API Misuses via Automatically Mining API Path Patterns},
year = {2024},
volume = {},
issn = {2375-1207},
pages = {43-43},
abstract = {Extracting API patterns from the source code has been extensively employed to detect API misuses. However, recent studies manually provide pattern templates as prerequisites, requiring prior software knowledge and limiting their extraction scope. This paper presents APP-Miner (API path pattern miner), a novel static analysis framework for extracting API path patterns via a frequent subgraph mining technique without pattern templates. The critical insight is that API patterns usually consist of APIs&#x27; data-related operations and are commonplace. Therefore, we define API paths as the control flow graphs composed of APIs&#x27; data-related operations, and thereby the maximum frequent subgraphs of the API paths are the probable API path patterns. We implemented APP-Miner and extensively evaluated it on four widely used open-source software: Linux kernel, OpenSSL, FFmpeg, and Apache httpd. We found 116, 35, 3, and 3 new API misuses from the above systems, respectively. Moreover, we gained 19 CVEs.},
keywords = {api-misuse;specification-inference;control-flow-graph;frequent-subgraph-mining;static-analysis},
doi = {10.1109/SP54263.2024.00043},
url = {https://doi.ieeecomputersociety.org/10.1109/SP54263.2024.00043},
publisher = {IEEE Computer Society},
address = {Los Alamitos, CA, USA},
month = {may}
}

```
