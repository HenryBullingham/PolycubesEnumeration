# PolycubesEnumeration
Parallel, optimized c++ code for enumerating polycubes inspired by [Mike's Cube Code] Computerphile video

# Requirements:

Cmake https://cmake.org/

A C++ 14 Compiler (tested only with msvc)

C++ build system for your os (ie make)

# Dependencies:

(Included in 3rdParty folder)

catch2 - for unit tests

popl - for command line parsing

# How to run:

make sure to build in release mode

then run with

PolyCubesThreadedTree.Exe -n POLYCUBE_SIZE -t NUM_THREADS

for example,

PolyCubesThreadedTree.Exe -n 10 -t 4

will search for the number of polycubes of size 10 with 4 worker threads

# Highlights of solution

* Uses rooted polycube method, so no global set to store cubes in
* Memory bounded -> Each thread allocates a fixed size of memory, and then requires no more heap space (about 2 MB per thread), meaning that the memory used is based on number of threads, not size of polycubes searched for
* Highly scalable - supports a large number of worker threads (could probably go up to 1000)

Note for using more worker threads than that: right now there's a pre-expansion step that finds potential polycubes of size 5, which have a max size limit. this means that there's a bounded number of workloads. for higher numbers of threads, this limit should be increased

# Results: 

On My machine, a 6 core intel i5, with 16 worker threads, I get the following times:

n = 9      0.11 s
n = 10     0.53 s
n = 11     3.52 s
n = 12    24.50 s