#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include <limits.h>

#include "mpi.h"

//função utilizada para ordenar usando a função qsort
int compare(const void * a, const void * b) {
   return(*(int*)a-*(int*)b);
}

//função que envia os elementos aos processos corretos com base nos pivos
int *troca_elementos(	int *vetor/*vetor a ser ordenado*/, 
						int tamanho/*tamanho do vetor*/, 
						int nthreads/*numero de threads e processos*/,
						int start/*onde comeca a parte do processo atual*/, 
						int size/*tamanho da parte do processo atual*/,
						int *pivots/*vetor de pivos*/) {

	int i, j, 	**envio/*matriz com todos os valores que vão ser enviados, cada linha 
						representa uma thread e a primeira coluna é o comprimento de cada linha*/,
				 rank/*numero do processo*/, 
				 *recebimento/*buffer para as mensagens revebidas
				 				o primeiro elemento é o comprimento*/,
				 *valores/*vetor com os valores finais pertencentes a cada processo
				 			o primeiro elemento é o comprimento*/;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);//pega o numero do processo

	//alocação  dos vetores e matriz	
	recebimento=(int *)malloc((tamanho+1)*sizeof(int));
	valores=(int *)malloc((tamanho+1)*sizeof(int));
	valores[0]=0;
	envio=(int **)malloc(nthreads*sizeof(int *));
	for(i=0;i<nthreads;i++) {
		envio[i]=(int *)malloc((tamanho+1)*sizeof(int));
		envio[i][0]=0;
	}

	//define para que processo deve ser enviado cada elemento do vetor
	//avaliando cada elemento em sua faixa (de start a start+size)
	for(i=start;i<start+size;i++) {
		for(j=0;j<nthreads-1;j++) {
			if(vetor[i]<=pivots[j]) {
				envio[j][0]++;
				envio[j][envio[j][0]]=vetor[i];
				break;
			}
		}
		if(vetor[i]>=pivots[nthreads-2]) {//quando pertence à ultima thread
			envio[nthreads-1][0]++;
			envio[nthreads-1][envio[nthreads-1][0]]=vetor[i];
		}
	}

	//cada processo envia a todos os outros seus elementos
	for(i=0;i<nthreads;i++) {
		if(i!=rank) {
			if(MPI_Send(envio[i], tamanho+1, MPI_INT, i, rank, MPI_COMM_WORLD)!=MPI_SUCCESS) {
				printf("Houve um erro no envio de uma mensagem.");
				MPI_Finalize();
				exit(1);
			}
		}
	}

	//cada processo recebe de todos os outros seus elementos e os guarda no vetor de retorno
	for(i=0;i<nthreads;i++) {
		if(i!=rank) {
			MPI_Recv(recebimento, tamanho+1, MPI_INT, i, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			for(j=1;j<recebimento[0]+1;j++) {
				valores[valores[0]+1]=recebimento[j];
				valores[0]++;
			}
		}
	}

	//os valores obtidos pelo próprio processo também são salvos
	for(i=1;i<envio[rank][0];i++) {
		valores[valores[0]+1]=envio[rank][i];
		valores[0]++;
	}

	//libreação de memória
	for(i=0;i<nthreads;i++) {
		free(envio[i]);
	}
	free(envio);
	free(recebimento);

	return valores;
}

//função a ser executada pelo processo 0
void master(int *vetor/*vetor com todos os elementos*/, 
			int tamanho/*comprimento do vetor*/, 
			int nthreads/*numero de threads e processos*/) {

	int i, j, tid/*numero da thread atual*/, 
			*size/*vetor que salva o início da partição de cada processo*/, 
			*start/*vetor que salva o tamanho da partição de cada processo*/,
			n/*valor temporario do numero de threads restante*/, 
			t/*valor temporario do tamanho restante*/, 
			soma=0/*valor temporário com o tamanho total utilizado*/, 
			*samples/*vetor de amostras obtidas de cada partição*/, 
			*pivots/*vetor de pivos*/, 
			aux=0/*valor temporário auxiliar para a reescrita do vetor ordenado*/, 
			**buffer/*matriz de buffer de recebimento
					cada linha é referente a um processo, a primeira coluna salva o comprimento*/;

	//alocações de memória	
	size=(int *)malloc(nthreads*sizeof(int));
	start=(int *)malloc(nthreads*sizeof(int));
	samples=(int *)malloc(nthreads*nthreads*sizeof(int));
	pivots=(int *)malloc((nthreads-1)*sizeof(int));
	buffer=(int **)malloc(nthreads*sizeof(int *));
	for(i=1;i<nthreads;i++) {
		buffer[i]=(int *)malloc((tamanho+1)*sizeof(int));
	}

	//faz o cálculo da partição de cada processo/thread
	n=nthreads;
	t=tamanho;
	for(i=0;i<nthreads;i++) {
		size[i]=ceil(t*1.0/n);
		n--;
		t-=size[i];
		start[i]=soma;
		soma+=size[i];
	}

	//porção paralela para obter as amostras do subvetor de cada processo/thread
	#pragma omp parallel private(tid, i) num_threads(nthreads)
	{
		tid=omp_get_thread_num();
		qsort(vetor+start[tid], size[tid], sizeof(int), compare);
		for(i=0;i<nthreads;i++) {
			samples[i+tid*nthreads]=vetor[start[tid]+i*(size[tid]/nthreads)];
		}
	}

	//ordena as amostras e obtem os pivos
	qsort(samples, nthreads*nthreads, sizeof(int), compare);
	for(i=0;i<nthreads-2;i++) {
		pivots[i]=samples[i*nthreads+nthreads/2-1];
	}
	pivots[nthreads-2]=samples[(nthreads-2)*nthreads+nthreads/2];

	//envia o vetor de pivos, e os dados da partição de cada processo
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

	//obtem os valores referentes ao processo master	
	buffer[0]=troca_elementos(vetor, tamanho, nthreads, start[0], size[0], pivots);

	//recebe os valores referentes a todos os outros processos e ordena todos os subvetores
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

	
	printf("\n\n\nTESTE\n\n\n");
	MPI_Finalize();
	exit(0);

	//salva os valores ordenados no vetor principal
	for(i=0;i<nthreads;i++) {
		for(j=1;j<=buffer[i][0];j++) {
			vetor[aux]=buffer[i][j];
			aux++;
		}
	}

	//libera a memória
	for(i=0;i<nthreads;i++) {
		free(buffer[i]);
	}
	free(buffer);
	free(size);
	free(start);
	free(samples);
	free(pivots);
}

//função executada por todos os outros processos
void slave(	int *vetor/*vetor com todos os valores*/, 
			int tamanho/*tamanho do vetor*/, 
			int nthreads/*numero de threads e processos*/) {

	int start/*começo da partição do processo*/,
		size/*tamanho da partição do processo*/, 
		*valores/*vetor retornado pela função de troca de elementos
				a primeira posição é o comprimento*/, 
		*pivots/*vetor de pivos*/;

	//alocaão de memória
	pivots=(int *)malloc((nthreads-1)*sizeof(int));
	MPI_Recv(pivots, nthreads-1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&start, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&size, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	//definição do vetor de valores e envio ao processo master
	valores=troca_elementos(vetor, tamanho, nthreads, start, size, pivots);
	if(MPI_Send(valores, tamanho+1, MPI_INT, 0, 3, MPI_COMM_WORLD)!=MPI_SUCCESS) {
		printf("Houve um erro no envio de uma mensagem.");
		MPI_Finalize();
		exit(1);
	}

	//libreação de memória
	free(pivots);
	free(valores);
}

int main(int argc, char **argv) {

	int *vetor/*vetor com todos os valores*/, i, 
		nthreads/*numero de processos/threads*/, 
		rank/*rank do processo atual*/, 
		tamanho/*comprimento do vetor*/;

	//o segundo argumento é o comprimento do vetor
	if(argc<2) {
		exit(0);
	}
	tamanho=atoi(argv[1]);

	//alocação de definição do vetor
	//o vetor é definido em ordem decrescente, começando em tamanho
	vetor=(int *)malloc(tamanho*sizeof(int));
	for(i=0;i<tamanho;i++) {
		vetor[i]=tamanho-i;
	}

	//inicia MPI, obtém o numero de threads e o rank
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nthreads);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//parte a ser executada pelo processo 0 (master)
	if(!rank) {
		master(vetor, tamanho-1, nthreads);
		for(i=0;i<tamanho;i++) {
			printf("%d, ", vetor[i]);
		}
		printf("%d.", vetor[i]);
	}

	//parte executada pelos outros processos
	else {
		slave(vetor, tamanho, nthreads);
	}

	//finaliza e libera memória
	MPI_Finalize();
	free(vetor);
	return 0;
}