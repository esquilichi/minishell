#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include "parser.h"


typedef struct job {
    pid_t pid;
    char comando[1024];
    tline *line;
} job;

int executePipes(int **matrix, tline *line, int *last_job, job array[]);
int firstPipe(int **matrix, tline* line, int nPipes);
int lastPipe(int **matrix, tline* line, int nPipes);
int mediumPipe(int **matrix, tline* line, int nPipes, int i);
int crearPipe(int **matrix, int statusPipe, int nPipes);
void matrixFree(int **matrix, int n);
void change_redirections(tline *line, int casito);
