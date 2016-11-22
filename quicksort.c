#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include <limits.h>

#include "mpi.h"

int compare(const void * a, const void * b) {
   return(*(int*)a-*(int*)b);
}

int *troca_elementos(int *vetor, int tamanho, int nthreads, int start, int size, int *pivots) {
	int i, j, k, **envio, rank, *recebimento, *valores;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	recebimento=(int *)malloc((tamanho+1)*sizeof(int));
	valores=(int *)malloc((tamanho+1)*sizeof(int));
	valores[0]=0;
	envio=(int **)malloc(nthreads*sizeof(int *));
	for(i=0;i<nthreads;i++) {
		envio[i]=(int *)malloc((tamanho+1)*sizeof(int));
		envio[i][0]=0;
	}
	for(i=start;i<start+size;i++) {
		for(j=0;j<nthreads-1;j++) {
			if(vetor[i]<=pivots[j]) {
				envio[j][0]++;
				envio[j][envio[j][0]]=vetor[i];
				break;
			}
		}
		if(vetor[i]>=pivots[nthreads-2]) {
			envio[nthreads-1][0]++;
			envio[nthreads-1][envio[nthreads-1][0]]=vetor[i];
		}
	}	
	for(i=0;i<nthreads;i++) {
		if(i!=rank) {
			if(MPI_Send(envio[i], tamanho+1, MPI_INT, i, rank, MPI_COMM_WORLD)!=MPI_SUCCESS) {
				printf("Houve um erro no envio de uma mensagem.");
				MPI_Finalize();
				exit(1);
			}
		}
	}
	for(i=0;i<nthreads;i++) {
		if(i!=rank) {
			MPI_Recv(recebimento, tamanho+1, MPI_INT, i, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			for(j=1;j<recebimento[0]+1;j++) {
				valores[valores[0]+1]=recebimento[j];
				valores[0]++;
			}
		}
	}
	for(i=1;i<envio[rank][0];i++) {
		valores[valores[0]+1]=envio[rank][i];
		valores[0]++;
	}
	for(i=0;i<nthreads;i++) {
		free(envio[i]);
	}
	free(envio);
	free(recebimento);
	return valores;
}

void master(int *vetor, int tamanho, int nthreads) {
	int i, j, tid, *size, *start, n, t, soma=0, *samples, *pivots, aux=0, **buffer;
	size=(int *)malloc(nthreads*sizeof(int));
	start=(int *)malloc(nthreads*sizeof(int));
	samples=(int *)malloc(nthreads*nthreads*sizeof(int));
	pivots=(int *)malloc((nthreads-1)*sizeof(int));
	buffer=(int **)malloc(nthreads*sizeof(int *));
	for(i=1;i<nthreads;i++) {
		buffer[i]=(int *)malloc((tamanho+1)*sizeof(int));
	}
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
	pivots[nthreads-2]=samples[(nthreads-2)*nthreads+nthreads/2];
	for(i=1;i<nthreads;i++) {
		if(MPI_Send(pivots, nthreads-1, MPI_INT, i, 0, MPI_COMM_WORLD)!=MPI_SUCCESS) {
			printf("Houve um erro no envio de uma mensagem.");
			MPI_Finalize();
			exit(1);
		}
		if(MPI_Send(&start[i], 1, MPI_INT, i, 1, MPI_COMM_WORLD)!=MPI_SUCCESS) {
			printf("Houve um erro no envio de uma mensagem.");
			MPI_Finalize();
			exit(1);
		}
		if(MPI_Send(&size[i], 1, MPI_INT, i, 2, MPI_COMM_WORLD)!=MPI_SUCCESS) {
			printf("Houve um erro no envio de uma mensagem.");
			MPI_Finalize();
			exit(1);
		}
	}
	buffer[0]=troca_elementos(vetor, tamanho, nthreads, start[0], size[0], pivots);
	#pragma omp parallel private(tid) num_threads(nthreads)
	{
		tid=omp_get_thread_num();
		if(!tid) {
			qsort(buffer[0]+1, buffer[0][0], sizeof(int), compare);
		}
		else {
			MPI_Recv(buffer[tid], tamanho+1, MPI_INT, tid, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			qsort(buffer[tid]+1, buffer[tid][0], sizeof(int), compare);
		}
	}
	for(i=0;i<nthreads;i++) {
		for(j=1;j<=buffer[i][0];j++) {
			vetor[aux]=buffer[i][j];
			aux++;
		}
	}
	for(i=0;i<nthreads;i++) {
		free(buffer[i]);
	}
	free(buffer);
	free(size);
	free(start);
	free(samples);
	free(pivots);
}

void slave(int *vetor, int tamanho, int nthreads) {
	int start, size, *valores, *pivots, **envio;
	pivots=(int *)malloc((nthreads-1)*sizeof(int));
	MPI_Recv(pivots, nthreads-1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&start, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&size, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	valores=troca_elementos(vetor, tamanho, nthreads, start, size, pivots);
	if(MPI_Send(valores, tamanho+1, MPI_INT, 0, 3, MPI_COMM_WORLD)!=MPI_SUCCESS) {
		printf("Houve um erro no envio de uma mensagem.");
		MPI_Finalize();
		exit(1);
	}
	free(pivots);
	free(valores);
}

int main(int argc, char **argv) {
	int *vetor, i, nthreads, rank, tamanho;
	if(argc<2) {
		exit(0);
	}
	tamanho=atoi(argv[1]);
	vetor=(int *)malloc(tamanho*sizeof(int));
	for(i=0;i<tamanho;i++) {
		vetor[i]=tamanho-i;
	}
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nthreads);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if(!rank) {
		master(vetor, tamanho, nthreads);
		for(i=0;i<tamanho;i++) {
			printf("%d ", vetor[i]);
		}
		printf("\n");
	}
	else {
		slave(vetor, tamanho, nthreads);
	}
	MPI_Finalize();
	free(vetor);
	return 0;
}