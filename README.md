# MPI Parallel Prim's Algorithm
Parallel Prim's Algorithm using MPI

## MPI

A program written in C using MPI to implement Prim's algorithm on parallel systems.

### Input file

The file in which you can enter the size of the adjacency matrix of the graph on which Prim's algorithm will be run is Input/Matrix.txt. In the file, you can enter the size of the adjacency matrix, then the program will generate such an adjacency matrix automatically (you should enter one number in the first line). You can also write the matrix yourself in the file. In this case, the first line writes the size of the matrix, and the following lines the matrix itself. IT IS REQUIRED that the matrix is ​​entered correctly. Input examples can be found in the Input directory.

### Output file

The adjacency matrix, the algorithm result and its execution time are visible in the file Output/Output.txt. Sample results can be seen in the Example files located in the Output directory.

### How to run program with Makefile:

To compile program, use:

```bash
make
```

To clean directory, use:

```bash
make clean
```

To run compiled program you need to create a "nodes" file in MPI directory containing the names of the nodes on which the program can be run, each on a new line, for example:
stud204-11
stud204-07
stud204-14

After creating file "nodes" run program with:

```bash
make post_build ARG=number_of_nodes
```

Where number_of_nodes is the number of nodes that will be used in the program.

### How to run program with Scripts (only in stud-204):

To compile program, create file "nodes" and run program on 3, 8 and 16 nodes, use:

```bash
./run.sh
```

To clean directory, use:

```bash
./clean.sh
```


