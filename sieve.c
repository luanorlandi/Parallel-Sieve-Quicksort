// compile with
// gcc -g -Wall -fopenmp -o sieve sieve.c -lm

// run with
// ./sieve 100

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <omp.h>

#define PRIME 0
#define NONPRIME 1

#define THREAD_COUNT 4

void crossout(int *prime, int size) {
	int i, j;

	#pragma omp parallel for num_threads(THREAD_COUNT) \
		private(j)
	for(i = 2; i < size; i++) {

		/* if is crossout, all his multiple
			it's already or being crossout */
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
	clock_t t;

	if(argc < 2) {
		printf("Need argument for range\n");
		exit(-1);
	}

	/* inclusive range */
	size = 1 + atoi(argv[1]);

	/* prime numbers will be 0 */
	/* not prime numbers will be 1 */
	prime = (int*) calloc(size, sizeof(int));

	t = clock();

	/* put 1 in not primes */
	crossout(prime, size);

	t = clock() - t;

	showAnswer(prime, size);
	printf("Crossout done in %.2lfs\n", (double) t / CLOCKS_PER_SEC);

	//free(prime);

	return 0;
}
