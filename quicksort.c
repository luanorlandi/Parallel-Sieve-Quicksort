#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>

#include "mpi.h"

#define NUMERO 27

int compare(const void * a, const void * b) {
   return(*(int*)a-*(int*)b);
}

void master(int *vetor, int tamanho, int nthreads) {
	int i, tid, *size, *start, n, t, soma=0, *pivots, x;
	size=(int *)malloc(nthreads*sizeof(int));
	start=(int *)malloc(nthreads*sizeof(int));
	pivots=(int *)malloc(nthreads*nthreads*sizeof(int));
	n=nthreads;
	t=tamanho;
	for(i=0;i<nthreads;i++) {
		size[i]=ceil(t*1.0/n);
		n--;
		t-=size[i];
		start[i]=soma;
		soma+=size[i];
	}
	#pragma omp parallel private(tid, i) num_threads(nthreads)
	{
		tid=omp_get_thread_num();
		qsort(vetor+start[tid], size[tid], sizeof(int), compare);
		for(i=0;i<nthreads;i++) {
			pivots[i+tid*nthreads]=vetor[start[tid]+i*(size[tid]/nthreads)];
		}
	}
	qsort(pivots, nthreads*nthreads, sizeof(int), compare);
	free(size);
	free(start);
	free(pivots);
}

int main(int argc, char **argv) {
	int *vetor, i, nthreads=3, rank;
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &nthreads);
	printf("%d\n\n", nthreads);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if(!rank) {
		vetor=(int *)malloc(NUMERO*sizeof(int));
		for(i=0;i<NUMERO;i++) {
			vetor[i]=NUMERO-i;
		}
		master(vetor, NUMERO, nthreads);
		for(i=0;i<NUMERO;i++) {
			printf("%d ", vetor[i]);
		}
		free(vetor);
	}
	MPI_Finalize();
	return 0;
}