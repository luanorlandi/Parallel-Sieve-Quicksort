// compile with
// gcc -g -Wall -fopenmp -o sieve sieve.c -lm

// run with
// ./sieve 100 0

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <omp.h>

int main(int argc, char *argv[]) {
	int *prime;		/* boolean */
	int size;
	int i = 0;

	if(argc < 2) {
		printf("Need argument for range\n");
	}

	/* inclusive range */
	size = 1 + atoi(argv[1]);

	/* prime numbers will be 0 */
	/* not prime numbers will be 1 */
	prime = (int*) calloc(size) * sizeof(int));

	/* put 1 in not primes */
	crossout(prime);

	free(prime);

	return 0;
}
