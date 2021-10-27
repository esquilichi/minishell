#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "parser.h"

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
int jobs_buffer[32][32];
int num_commands = 0;


// Definición de funciones
int check_command(char * filename);
void my_cd(tline *line);
char *make_prompt(uid_t uid);


//Shell
int main(int argc, char const *argv[]){

	uid_t uid = getuid();
	const char *prompt = make_prompt(uid); // Movida para tener prompt con whoami
	pid_t pid;
	tline *line; // Struct de parser.h
	char *buffer = (char *) malloc(1024*(sizeof(char))); // Memoria dinamica porque why not
	printf("%s", prompt);
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
			}if (strcmp(line->commands[0].argv[0],EXIT) == 0){ // Salimos de la shell
				exit(0);
			}if (strcmp(line->commands[0].argv[0],JOBSCONST)){ // JOBS con fg y bg

			}if (strcmp(line->commands[0].argv[0],G_USAGE) == 0){ // Futuro Global Usage

			}else{
				//Creamos el hijo para ejecutar el comando

				pid = fork();
				if (pid < 0){
					fprintf(stderr,"Falló el fork");
					exit(1);
				}else if(pid == 0){ //Hijo
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
						exit(0);
					}
				}else{ //Padre 
					wait(&exit_status);
					if(WIFEXITED(exit_status) != 0){
						if (WEXITSTATUS(exit_status) != 0){
							fprintf(stdout,"El comando ha tenido un código de estado erróneo\n");
						}
					}
					signal(SIGINT,  SIG_IGN); //Ignorar señales en Padre
					signal(SIGQUIT, SIG_IGN);
					signal(SIGTSTP, SIG_IGN);
				}
			}
		}else{ //Tenemos comandos separados por | (pipes)
			/* code */
		}
		printf("%s", prompt);
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

char * make_prompt(uid_t uid){
	FILE *fp;
	char *path = (char *) malloc(128* sizeof(char));

	fp = popen("whoami", "r");
	if (fp == NULL){
		fprintf(stderr, "Error al ejecutar whoami");
		exit(1);
	}

	fgets(path, sizeof(path), fp);
	path[strcspn(path, "\n")] = 0;
	pclose(fp);
	if (uid == 0)
		strcat(path,"# ");
	else
		strcat(path,"$ ");
	return path;
}

void my_cd(tline *line){
	char* dir;
	char pwd[512];

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
	}

	if (chdir(dir) != 0){ // Si la función devuelve algo que no es 0, falló
		fprintf(stderr,"No existe el directorio %s\n", dir);
	}
}



