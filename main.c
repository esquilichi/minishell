/*
 ____________________________________
/ Hecho por Clara Contreras e Ismael \
\ Esquilichi                         /
 ------------------------------------
        \   ^__^
         \  (oo)\_______
            (__)\       )\/\
                ||----w |
                ||     ||
*/




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include "parser.h"
#include "colors.h"
#include "mypipes.h"


/*
 _________
< Defines >
 ---------
        \   ^__^
         \  (oo)\_______
            (__)\       )\/\
                ||----w |
                ||     ||
*/
#define True 1
#define False 0
#define ERROR -1
#define CDCONST "cd"
#define EXIT "exit"
#define JOBSCONST "jobs"
#define G_USAGE "globalusage"

/*
 ____________________
< Variables Globales >
 --------------------
        \   ^__^
         \  (oo)\_______
            (__)\       )\/\
                ||----w |
                ||     ||
*/
char *user;
char hostname[64];
int jobs_buffer[32][32];
int num_commands = 0;
uid_t uid;
int **pipes_matrix;
/*
 __________________________
< Definición de funciones >
 --------------------------
        \   ^__^
         \  (oo)\_______
            (__)\       )\/\
                ||----w |
                ||     ||
*/

int check_command(char * filename);
int executePipes(tline *line);
void my_cd(tline *line);
void make_prompt();
void print_promt(int exit_code);
void change_redirections(tline *line, int casito);
void matrixFree(int **matrix, int n);

/*
 _________
< Código >
 ---------
        \   ^__^
         \  (oo)\_______
            (__)\       )\/\
                ||----w |
                ||     ||
*/

int main(int argc, char const *argv[]){
	make_prompt();
	pid_t pid;
	tline *line; // Struct de parser.h
	char *buffer = (char *) malloc(1024*(sizeof(char))); // Memoria dinamica porque why not
	print_promt(0);
	int exit_status = 0; //Se usa para comprobar salida de estado del hijo


	signal(SIGINT,  SIG_IGN); //Hay que ignorar Ctrl+C y Ctrl+Z en la shell
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);


	while(fgets(buffer, 1024, stdin)){
		signal(SIGINT,  SIG_IGN); //Hay que ignorar Ctrl+C y Ctrl+Z en la shell
		signal(SIGQUIT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);

		line = tokenize(buffer);
		if (line == NULL){
			continue;
		}
		if (line->redirect_input != NULL){
			//printf("%s\n", line->redirect_input);
		}
		if (line->redirect_output != NULL){
			//change_redirections(line);
		}
		if (line->redirect_error != NULL){
			printf("%s\n", line->redirect_error);
		}
		if (line->background){ // En el parser.h está definido como int/bool
			/* code */
		}
		if(line->ncommands == 1){

			//Restablecemos las señales a las que están por defecto
			if (strcmp(line->commands[0].argv[0],CDCONST) == 0){ // Miramos si el comando que queremos es cd
				my_cd(line); // Aquí no hacemos hijo o no cambiamos de dir
			}else if (strcmp(line->commands[0].argv[0],EXIT) == 0){ // Salimos de la shell
				exit(0);
			}else if (strcmp(line->commands[0].argv[0],JOBSCONST) == 0){ // JOBS con fg y bg

			}else if (strcmp(line->commands[0].argv[0],G_USAGE) == 0){ // Futuro Global Usage

			}else{
				//Creamos el hijo para ejecutar el comando

				pid = fork();
				if(pid<0){
					pid = ERROR; //En caso de error
				}
				switch (pid) {
					case 0:
						signal(SIGINT,  SIG_DFL);
						signal(SIGKILL, SIG_DFL);
						signal(SIGTSTP, SIG_DFL);
						if (line->redirect_output != NULL)
							change_redirections(line, 1);
						if (line->redirect_input != NULL)
							change_redirections(line, 0);
						if (line->redirect_error != NULL)
							change_redirections(line, 2);
						char *command = line->commands[0].filename; // Path absoluto
						//printf("%s\n", command); Debug Filename
						if (check_command(command)){
							execvp(command, line->commands[0].argv);
							fprintf(stderr,"No se ha podido ejecutar el comando %s\n",command);
						}else{
							//printf("Mi pid es == %d\n",pid);
							fprintf(stderr,"No se encuentra el comando %s\n",line->commands[0].argv[0]);
							exit(1);
						}
					break;

					case ERROR:
						fprintf(stderr,"Falló el fork");
						exit(1);
					break;

					default:
						wait(&exit_status);
						if(WIFEXITED(exit_status) != 0){
							//printf("DEBUG status: %d\n",exit_status);
							if (WEXITSTATUS(exit_status) != 0){
								fprintf(stdout,"El comando ha tenido un código de estado erróneo\n");
							}
						}
						signal(SIGINT,  SIG_IGN); //Ignorar señales en Padre
						signal(SIGQUIT, SIG_IGN);
						signal(SIGTSTP, SIG_IGN);
					break;
				}
			}
		}else if (line->ncommands >= 2){ //executePipes
			executePipes(line);
		}
		print_promt(exit_status);
	}


	free(buffer);
	return 0;
}

int executePipes(tline *line){
	int nPipes = line->ncommands - 1;
	pid_t pid;
	int statusPipe = 0;
	int dupStatus = 0;
	int execStatus = 0;
	int status = 0;
	pipes_matrix = (int **) malloc(nPipes * sizeof(int *));

	/* ----- Reservar memoria para la matriz ----- */
	for (int i = 0; i < nPipes; ++i){
		pipes_matrix[i] = malloc(2 * sizeof(int));
	}

	/* Crear pipes para los n comandos a ejecutar */
	for (int i = 0; i < nPipes; ++i){
		statusPipe = pipe(pipes_matrix[i]);
		if (statusPipe < 0){
			perror("No se ha podido generar de forma correcta el pipe\n");
			matrixFree(pipes_matrix, nPipes);
			return ERROR;
		}
	}

	for (int i = 0; i < line->ncommands; ++i){
		/* printf("%s\n",line->commands[i].filename); */
		if (line->commands[i].filename == NULL){ // Comando no válido
			fprintf(stderr,"No se encuentra el comando %s\n",line->commands[i].argv[0]);
			matrixFree(pipes_matrix, nPipes);
			return ERROR;
		}
		pid = fork();
		if(pid == 0){ // Hijo
			// Ejecutar comando y pasar output al input del pipe
			signal(SIGINT,  SIG_DFL);
			signal(SIGKILL, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
			/* Tenemos varios casos
				1. Primer Comando
				2. Último comando (stdout a pantalla)
				3. Comando intermedio */
			if (i == 0){ // Primer Comando
				if(line->redirect_input != NULL){ // input redirect
					change_redirections(line, 0);
				}
				// Hay que cerrar todos los pipes que no se vayan a usar
				for (int j = 1; j < nPipes; ++j){
					close(pipes_matrix[j][0]);
					close(pipes_matrix[j][1]);
				}
				// Cerramos stdin del pipe que vamos a usar
				close(pipes_matrix[0][0]);
				dup2(pipes_matrix[0][1],STDOUT_FILENO);
				close(pipes_matrix[0][1]);

			}
			else if(i == nPipes){ // Último comando
					for (int k = 0; k < nPipes - 1; k++) {
					close(pipes_matrix[k][0]);
					close(pipes_matrix[k][1]);
				}
				close(pipes_matrix[i - 1][1]); // Cerrar entrada de pipe anterior
				dup2(pipes_matrix[i - 1][0],STDIN_FILENO); //Asignar salida del pipe anterior
				close(pipes_matrix[i - 1][0]); // Cerrar otro extremo del pipe
				//Redirección de salida
				if (line->redirect_output != NULL){
					change_redirections(line,1);
				}
				else if(line->redirect_error != NULL){
					change_redirections(line,2);
				}
			} 
			else { //Comando intermedio
				for (int p = 0; p < nPipes; ++p){
					if ((p != i) && (p != (i-1))){
						close(pipes_matrix[p][0]);
						close(pipes_matrix[p][1]);
					}
				}
				close(pipes_matrix[i - 1][1]);
				close(pipes_matrix[i][0]);
				dup2(pipes_matrix[i-1][0],STDIN_FILENO); // Pillar salida del pipe anterior
				close(pipes_matrix[i-1][0]); // Cerrar extremo del pipe
				dup2(pipes_matrix[i][1],STDOUT_FILENO); // Mandar mi salida por salida del pipe
				close(pipes_matrix[i][1]); // Cerrar extremo del pipe 
			}

			execStatus = execvp(line->commands[i].filename, line->commands[i].argv);
			if(execStatus < 0){
				fprintf(stderr, "La has liado");
				return ERROR;
			}
		}
	}

	// Código solo alcanzado por el padre
	for (int w = 0; w < nPipes; ++w){
		close(pipes_matrix[w][0]);
		close(pipes_matrix[w][1]);
	}

	for (int i = 0; i < line->ncommands; ++i)	{
		wait(&status);
	}

	matrixFree(pipes_matrix, nPipes);
	return WEXITSTATUS(status);
}

int check_command(char *filename){
	if (filename != NULL){
		return True;
	} else {
		return False;
	}
}

void make_prompt(){
	char l_hostname[64];
	uid = getuid();
	gethostname(l_hostname, sizeof(l_hostname));
	strcpy(hostname, l_hostname);
	user = getenv("USER");
}

void my_cd(tline *line){
	char* dir;
	int dir_status = ERROR;
	if (line->commands[0].argc > 2){
		fprintf(stderr, "Más de dos argumentos\n");
	}

	if (line->commands[0].argc == 1){ // Solo me pasan cd, sin args
		dir = getenv("HOME");
		if (dir == NULL){
			fprintf(stderr, "$HOME no tiene un valor\n");
		}
	}else{
		dir = line->commands[0].argv[1];
		//printf("DEBUG: %s\n",dir);
	}

	dir_status = chdir(dir);
	if (dir_status){ // Si la función devuelve algo que no es 0, falló
		fprintf(stderr,"No existe el directorio %s\n", dir);
	}
}

void print_promt(int exit_code){
	char cwd[256];
	getcwd(&cwd[0], sizeof(cwd));
	cyan();
	printf("%s", user);
	reset();
	printf("@");
	cyan();
	printf("%s", hostname);
	reset();
	printf(" ");
	magenta();
	printf("%s", cwd);
	reset();
	printf(" ");
		if (exit_code != 0)
	{
		red();
		printf("status: %d ", exit_code);
		reset();
	}
	if (uid != 0)
		printf("$");
	else{
		red();
		printf("#");
		reset();
	}
			
	printf(" ");

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
		}

}

void matrixFree(int **matrix, int n) {
	int i;
	for(i = 0; i < n; i++) {
		free(matrix[i]);
	}
	free(matrix);
}




