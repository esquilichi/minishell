#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include "parser.h"


#define MAX_JOBS 256
typedef struct job {
    pid_t pid;
    char comando[MAX_JOBS];
    int eliminado;
} job;

int executePipes(int **matrix, tline *line, job array[], char *buffer);
int firstPipe(int **matrix, tline* line, int nPipes);
int lastPipe(int **matrix, tline* line, int nPipes);
int mediumPipe(int **matrix, tline* line, int nPipes, int i);
int crearPipe(int **matrix, int statusPipe, int nPipes);
void matrixFree(int **matrix, int n);
void change_redirections(tline *line, int casito);
