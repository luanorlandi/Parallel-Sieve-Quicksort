// compile with
// gcc -g -Wall -o main main.c

// run with
// ./main n t

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	char argument[5][100];
	argument[0] = "quicksort";
	argument[1] = "-mpirun";
	argument[2] = "-np";
	argument[3] = argv[1];
	argument[4] = argv[2];

	execvp("quicksort", argument);

	return 0;
}
