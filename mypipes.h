#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include "parser.h"

int fistPipe(int **matrix, tline* line, int nPipes);
int lastPipe(int **matrix, tline* line, int nPipes);
int mediumPipe(int **matrix, tline* line, int nPipes);