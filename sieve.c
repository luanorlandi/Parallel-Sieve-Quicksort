// compile with
// gcc -g -Wall -fopenmp -o sieve sieve.c -lm

// run with
// ./sieve 100 0

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <omp.h>

#define PRIME 0
#define NONPRIME 1

void crossout(int *prime, int size) {
	int i, j;

	for(i = 2; i < size; i++) {
		if(prime[i] == PRIME) {
			for(j = i+i; j < size; j += i) {
				prime[j] = NONPRIME;
			}
		}
	}
}

void showAnswer(int *prime, int size) {
	int total = 0, i;

	printf("Prime numbers found (2 to %d):\n", size-1);

	for(i = 2; i < size; i++) {
		if(prime[i] == PRIME) {
			printf("%d ", i);
			total++;
		}
	}

	printf("\nTotal: %d\n", total);
}

int main(int argc, char *argv[]) {
	int *prime;		/* boolean */
	int size;

	if(argc < 2) {
		printf("Need argument for range\n");
		exit(-1);
	}

	/* inclusive range */
	size = 1 + atoi(argv[1]);

	/* prime numbers will be 0 */
	/* not prime numbers will be 1 */
	prime = (int*) calloc(size, sizeof(int));

	/* put 1 in not primes */
	//#pragma omp parallel num_threads(4)
	crossout(prime, size);

	showAnswer(prime, size);

	free(prime);

	return 0;
}
