#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "parser.h"
#include "colors.h"

/*
Cosas a hacer:
Permitir ejecución de comando con background &
Implementar Jobs con array de procesos
Manejar redirecciones

*/

// Defines
#define True 1
#define False 0
#define CDCONST "cd"
#define EXIT "exit"
#define JOBSCONST "jobs"
#define G_USAGE "globalusage"


// Variables globales
char *user;
char hostname[64];
int jobs_buffer[32][32];
int num_commands = 0;
uid_t uid;

// Definición de funciones
int check_command(char * filename);
void my_cd(tline *line);
void make_prompt();
void print_promt();


//Shell
int main(int argc, char const *argv[]){
	make_prompt();
	pid_t pid;
	tline *line; // Struct de parser.h
	char *buffer = (char *) malloc(1024*(sizeof(char))); // Memoria dinamica porque why not
	print_promt();
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
			/* code */
		}
		if (line->redirect_output != NULL){
			/* code */
		}
		if (line->redirect_error != NULL){
			/* code */
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
					pid = -1; //En caso de error
				}
				switch (pid) {
					case 0:
						signal(SIGINT,  SIG_DFL);
						signal(SIGKILL, SIG_DFL);
						signal(SIGTSTP, SIG_DFL);
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

					case -1:
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
		}else if (line->ncommands == 2){ //Tenemos comandos separados por | (pipes)
			printf("Tengo dos comandos jeje\n");
		}
		print_promt();
	}


	free(buffer);
	return 0;
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
	int dir_status = -1;
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

void print_promt(){
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
	if (uid != 0)
		printf("$");
	else{
		red();
		printf("#");
		reset();
	}
			
	printf(" ");
}


