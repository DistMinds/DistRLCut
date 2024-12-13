# README for DistRLCut Code  

## Overview  
This repository contains a C++ implementation of a distributed graph partitioning algorithm. The implementation requires **MPI** for communication across machines and **OpenMP** for parallel processing within each machine.  

## Prerequisites  
1. Install **MPI** on all machines.  
2. Ensure **OpenMP** is supported in your compiler (e.g., `g++` or `clang++`).  

## Preparing the Graph Data  

### ASCII Graph File Format  
Prepare an ASCII graph file on each machine with the following format:  
```
vertex1 vertex2  
vertex2 vertex3  
...  
```
Each line represents an edge in the graph, where `vertex1` and `vertex2` are connected.  

### Binary Graph File Format  
To accelerate graph loading, we use a custom binary format. The structure is as follows:  
- The first 4 bytes: a magic number (`1145141919`) to identify the file.  
- The next 4 bytes: the total number of vertices (unsigned int).  
- The next 8 bytes: the total number of edges (unsigned long).  
- The remaining data: pairs of 8 bytes (4 bytes for each vertex in the edge).  

#### Diagram of Binary Structure:  
```
+----------------+----------------+----------------+-------------------+
| Magic Number   | # Vertices    | # Edges        | Edge List         |
| (4 bytes)      | (4 bytes)     | (8 bytes)      | (8 bytes per edge)|
+----------------+----------------+----------------+-------------------+
```

For example:  
If the graph has 2 vertices and 1 edge (connecting vertex 1 and vertex 2):  
```
Magic Number: 1145141919  
# Vertices: 2  
# Edges: 1  
Edge List: (1, 2)  
```

Binary representation in hex:  
```
1145141919 (4 bytes) | 2 (4 bytes) | 1 (8 bytes) | 1, 2 (8 bytes total)  
```

## Compilation  
Run the following command to compile the code:  
```bash
make
```

## Running the Program  
Run the program with the necessary parameters. (Details for the parameters to be added.)  

```bash
mpirun -n 4 -f nodes ./main -g "web-google.bin" -n "network/Amazon5.txt" -r 0.4 -w 1 -p 1 -z pagerank -o 300000
```

- -g : graph file path
- -n : network file path
- -r : budget rate
- -w : initial agent partition in work
  - 0 : hash
  - 1  : range
  - 2 : random
- -p : initial vertex partition
- -z : graph algorithm
  - pagerank
  - sssp
  - subgraph
- -o : overhead limit(second)

## Notes  
- Ensure that the graph files (ASCII or binary) are prepared and distributed to the appropriate machines.  
- The binary format is recommended for large graphs to reduce loading time.  

Feel free to modify the code or open an issue for any questions!  