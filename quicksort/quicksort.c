// compile with
// mpicc -g -Wall -fopenmp -o quicksort quicksort.c -lm

// run with
// ./quicksort -mpirun -np n t

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>

#include "mpi.h"

#define TAG 0

int compare(const void * a, const void * b) {
   return(*(int*)a-*(int*)b);
}

void troca_elementos(int *vetor, int tamanho, int nthreads, int start, int size, int *pivots) {
	int i, j, k, **envio, rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	envio=(int **)malloc(nthreads*sizeof(int *));
	for(i=0;i<nthreads;i++) {
		envio[i]=(int *)malloc(tamanho*sizeof(int));
		envio[i][0]=0;
	}
	for(i=start;i<start+size;i++) {
		for(j=0;j<nthreads-1;j++) {
			if(vetor[i]<pivots[j]) {
				envio[j][0]++;
				envio[j][envio[j][0]]=vetor[i];
				break;
			}
		}
		if(vetor[i]>pivots[nthreads-1]) {
			envio[nthreads-1][0]++;
			envio[nthreads-1][envio[nthreads-1][0]]=vetor[i];
		}
	}
	//codigo para envio e recebimento da matriz envio
	for(i=0;i<nthreads;i++) {
		free(envio[i]);
	}
	free(envio);
}

void master(int *vetor, int tamanho, int nthreads) {
	int i, tid, *size, *start, n, t, soma=0, *samples, *pivots, x;
	size=(int *)malloc(nthreads*sizeof(int));
	start=(int *)malloc(nthreads*sizeof(int));
	samples=(int *)malloc(nthreads*nthreads*sizeof(int));
	pivots=(int *)malloc((nthreads-1)*sizeof(int));
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
			samples[i+tid*nthreads]=vetor[start[tid]+i*(size[tid]/nthreads)];
		}
	}
	qsort(samples, nthreads*nthreads, sizeof(int), compare);
	for(i=0;i<nthreads-2;i++) {
		pivots[i]=samples[i*nthreads+nthreads/2-1];
	}
	pivots[i]=samples[i*nthreads+nthreads/2];
	for(i=1;i<nthreads;i++) {
		if(MPI_Send(pivots, nthreads-1, MPI_INT, i, TAG, MPI_COMM_WORLD)!=MPI_SUCCESS) {
			printf("Houve um erro no envio de uma mensagem.");
			MPI_Finalize();
			exit(1);
		}
		if(MPI_Send(&start[i], 1, MPI_INT, i, TAG, MPI_COMM_WORLD)!=MPI_SUCCESS) {
			printf("Houve um erro no envio de uma mensagem.");
			MPI_Finalize();
			exit(1);
		}
		if(MPI_Send(&size[i], 1, MPI_INT, i, TAG, MPI_COMM_WORLD)!=MPI_SUCCESS) {
			printf("Houve um erro no envio de uma mensagem.");
			MPI_Finalize();
			exit(1);
		}
	}
	free(size);
	free(start);
	free(samples);
	free(pivots);
}

void slave(int *vetor, int tamanho, int nthreads) {
	int start, size, *valores, *pivots, **envio;
	pivots=(int *)malloc((nthreads-1)*sizeof(int));
	MPI_Recv(pivots, nthreads-1, MPI_INT, 0, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&start, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&size, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	free(pivots);
}

int main(int argc, char **argv) {
	int *vetor, i, nthreads, rank, tamanho=27;
	vetor=(int *)malloc(tamanho*sizeof(int));
	for(i=0;i<tamanho;i++) {
		vetor[i]=tamanho-i;
	}
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &nthreads);
	printf("%d\n\n", nthreads);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if(!rank) {
		master(vetor, tamanho, nthreads);
	}
	else
		slave(vetor, tamanho, nthreads);
	MPI_Finalize();
	free(vetor);
	return 0;
}
