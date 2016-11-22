# Parallel Sieve of Eratosthenes and Quicksort
Project to make the Sieve of Eratosthenes and Quicksort algorithm with OpenMP and OpenMPI.

## Sieve of Eratosthenes

### Compile

The project uses OpenMP, a open library for parallel programming. You will need it to compile.

```bash
  $ gcc -g -Wall -fopenmp -o sieve sieve.c -lm
```

### Run

```bash
  $ ./sieve n t
```

'n' is the maximum number for the sieve
't' is the number of threads

## Quicksort

The PSRS (Parallel sorting by regular sampling) algorithm is implemented for this.
The project uses OpenMP and OpenMPI, open libraries for parallel programming. You will need both to compile.

### Compile

The project uses OpenMP, a open library for parallel programming. You will need it to compile.

```bash
  $ mpicc -g -Wall -fopenmp -o quicksort quicksort.c -lm
```

### Run

```bash
  $ ./quicksort -mpirun -np n t
```

'n' in the number of processes
't' is the number of threads
