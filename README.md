# Ring-KNN

Repository for the source code of the engine presented in the paper Worst-Case-Optimal Similarity Joins on Graph Databases

## Instructions

To run our code, **we have to install an extended version of the library SDSL**. Go to [this repository](https://github.com/darroyue/sdsl-lite) and follow the instructions.

After the extended version of SDSL is installed, we have to clone this repository and follow these steps:

1. Create our `build` folder and compile the code:
```Bash
mkdir build
cd build
cmake ..
make
```

Check that there is no errors.

2. Download our dataset:

- [Wikidata IMGPedia](https://figshare.com/s/889a0fc62ab4f655b593).

Now put both files inside a folder.

3. Building the index. After compiling the code we should have an executable called `build-index-similarity` in `build`. Now run:

```Bash
./build-index-similarity <absolute-path-to-file> <type-ring>
```

`<type-ring>` can take two values: `ring-knn` or `c-ring-knn`. Both are implementations of our ring index but using plain and compressed bitvectors, respectively.
This will generate the index in the folder where the `.dat` file is located. The index is suffixed with `.ring-knn` or `.c-ring-knn` according to the second argument.

4. Querying the index. In `build` folder, you should find another executable file called `query-index-similarity`. To solve the queries you should run:

```Bash
./query-index-similarity <absoulute-path-to-the-index-file> <absolute-path-to-the-query-file>
```

Note that the second argument is the path to a file that contains all the queries. The queries of our benchmark are in `queries`.

After running that command, you should see the number of the query, the number of results, and the elapsed time of each one of the queries with the following format:
```Bash
<query number>;<number of results>;<elapsed time>
```
