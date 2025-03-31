# MPI Parallel Prim's Algorithm
Parallel Prim's Algorithm using MPI

## MPI

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


