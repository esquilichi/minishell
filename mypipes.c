#include "mypipes.h"
#include <unistd.h>

int executePipes(int **matrix, tline *line,job array[], char* buffer){
	int nPipes = line->ncommands - 1;
	pid_t pid;
	int statusPipe = 0;
	int execStatus = 0;
	int status = 0;
	matrix = (int **) malloc(nPipes * sizeof(int *));

	/* ----- Reservar memoria para la matriz ----- */
	for (int i = 0; i < nPipes; ++i){
		matrix[i] = malloc(2 * sizeof(int));
	}

	/* Crear pipes para los n comandos a ejecutar */
	crearPipe(matrix, statusPipe, nPipes);
	pid_t pid1; 
	for (int i = 0; i < line->ncommands; ++i){
		/* printf("%s\n",line->commands[i].filename); */
		if (line->commands[i].filename == NULL){ // Comando no válido
			fprintf(stderr,"No se encuentra el comando %s\n",line->commands[i].argv[0]);
			matrixFree(matrix, nPipes);
			return -1;
		}
		pid = fork();
		if(pid>0 && i ==0)//El padre sabe el pid
			//pidG=pid;
			pid1=pid;

		if(pid == 0){ // Hijo
			if(line->background){
				if (i == 0){//Primer hijo
					setpgid(0,0);
				}else{
					setpgid(0, pid1);
				}
			}
            

			// Ejecutar comando y pasar output al input del pipe
			signal(SIGINT,  SIG_DFL);
			signal(SIGKILL, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
			/* Tenemos varios casos
				1. Primer Comando
				2. Último comando (stdout a pantalla)
				3. Comando intermedio */
			if (i == 0){ // Primer Comando
				firstPipe(matrix, line, nPipes);
			}
			else if(i == nPipes){ // Último comando

				lastPipe(matrix , line, nPipes);
			} 
			else { //Comando intermedio
				mediumPipe(matrix, nPipes, i);
			}

			execStatus = execvp(line->commands[i].filename, line->commands[i].argv);
			if(execStatus < 0){
				fprintf(stderr, "La has liado");
				return -1;
			}
		}
		/* if(!line-> background){
			for (int i = 0; i < line->ncommands; ++i)	{// Esperamos a q se ejecuten los comandos en orden
				waitpid(pid, &execStatus, 0);
			}
		} */
		
	}
    // Código solo alcanzado por el padre
    if (!line->background){
        for (int w = 0; w < nPipes; ++w){
            close(matrix[w][0]);
            close(matrix[w][1]);
        }

        for (int i = 0; i < line->ncommands; ++i)	{
            waitpid(pid, &execStatus, 0);
        }
    } else{
        int counter = -1;
        for (int i = 0; i < MAX_JOBS; ++i) {
            if(array[i].eliminado == 0){
                counter = i; // Última posición del array en la que tenemos un job
            }
        }
        array[counter + 1].pid = pid;
        //array[counter + 1].pgid = pid;
		array[counter + 1].pgid = pid1;
        buffer[strcspn(buffer, "\n")] = 0;
        strcpy(array[counter + 1].comando, buffer);
        array[counter + 1].eliminado = 0;
        fprintf(stdout, "[%d] %d\n", counter + 1, pid);
    }


	matrixFree(matrix, nPipes);
	return WEXITSTATUS(status);
}

int crearPipe(int **matrix, int statusPipe, int nPipes){
	for (int i = 0; i < nPipes; ++i){
		statusPipe = pipe(matrix[i]);
		if (statusPipe < 0){
			perror("No se ha podido generar de forma correcta el pipe\n");
			matrixFree(matrix, nPipes);
			return -1;
		}
   	}
   	return 0;
}

int firstPipe(int **matrix, tline* line, int nPipes){
	if(line->redirect_input != NULL){ // input redirect
		change_redirections(line, 0);
	}
				// Hay que cerrar todos los pipes que no se vayan a usar
	for (int j = 1; j < nPipes; ++j){
		close(matrix[j][0]);
		close(matrix[j][1]);
	} 
	close(matrix[0][0]); // Cerramos stdin del pipe que vamos a usar
	dup2(matrix[0][1],STDOUT_FILENO);
	close(matrix[0][1]);
	return 0;
}


int lastPipe(int **matrix, tline* line, int nPipes){
	for (int k = 0; k < nPipes - 1; k++) {
		close(matrix[k][0]);
		close(matrix[k][1]);
	}
	close(matrix[nPipes - 1][1]); // Cerrar entrada de pipe anterior
	dup2(matrix[nPipes - 1][0],STDIN_FILENO); //Asignar salida del pipe anterior
	close(matrix[nPipes - 1][0]); // Cerrar otro extremo del pipe
	//Redirección de salida
	if (line->redirect_output != NULL){
		change_redirections(line,1);
	}
	else if(line->redirect_error != NULL){
		change_redirections(line,2);
	}
	return 0;
}
int mediumPipe(int **matrix, int nPipes, int i){
	for (int p = 0; p < nPipes; ++p){
		if ((p != i) && (p != (i-1))){
			close(matrix[p][0]);
			close(matrix[p][1]);
		}
	}
	close(matrix[i - 1][1]);
	close(matrix[i][0]);
	dup2(matrix[i-1][0],STDIN_FILENO); // Pillar salida del pipe anterior
	close(matrix[i-1][0]); // Cerrar extremo del pipe
	dup2(matrix[i][1],STDOUT_FILENO); // Mandar mi salida por salida del pipe
	close(matrix[i][1]); // Cerrar extremo del pipe 
	return 0;
}

void matrixFree(int **matrix, int n) {
	int i;
	for(i = 0; i < n; i++) {
		free(matrix[i]);
	}
	free(matrix);
}

void change_redirections(tline *line, int casito){
	int file;
	switch (casito){
		case 0:
			// Redirección de entrada
			file = open(line->redirect_input, O_RDONLY);
			dup2(file, STDIN_FILENO);
			break;
		case 1:
			// Redirección de salida
			file = open(line->redirect_output, O_RDWR | O_CREAT, 0664);
			dup2(file, STDOUT_FILENO);
			break;
		case 2:
			// Redirección de error
			file = open(line->redirect_error, O_RDWR | O_CREAT, 0664);
			dup2(file, STDERR_FILENO);
			break;
        default:
            fprintf(stderr,"¿Que ha pasado aquí?");
		}

}

