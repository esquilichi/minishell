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
#define JOBS "jobs"
#define MAX_BGProcesses 20

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
uid_t uid;
int **pipes_matrix;
job jobs_array[MAX_BGProcesses];
int last_job = -1;

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

int check_command(const char *filename);

void my_cd(tline *line);

void my_jobs();

void make_prompt();

void print_promt(int exit_code);

void childHandler(int signal);

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

int main(int argc, char const *argv[]) {
    make_prompt();
    tline *line; // Struct de parser.h
    char *buffer = (char *) malloc(1024 * (sizeof(char))); // Memoria dinamica porque why not
    print_promt(0);
    int exit_status = 0; //Se usa para comprobar salida de estado del hijo
    int foreground_pid = 0;
    while (fgets(buffer, 1024, stdin)) {
        signal(SIGINT, SIG_IGN); //Hay que ignorar Ctrl+C y Ctrl+Z en la shell
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGCHLD, childHandler);
        line = tokenize(buffer);
        if (line == NULL) {
            continue;
        }
        if (line->ncommands == 1) {
            //Restablecemos las señales a las que están por defecto
            if (strcmp(line->commands[0].argv[0], CDCONST) == 0) { // Miramos si el comando que queremos es cd
                my_cd(line); // Aquí no hacemos hijo o no cambiamos de dir
            } else if (strcmp(line->commands[0].argv[0], EXIT) == 0) { // Salimos de la shell
                exit(0);
            } else if (strcmp(line->commands[0].argv[0], JOBSCONST) == 0) { // JOBS con fg y bg
                my_jobs();
            } else if (strcmp(line->commands[0].argv[0], G_USAGE) == 0) { // Futuro Global Usage

            } else {
                //Creamos el hijo para ejecutar el comando
                pid_t pid;
                pid = fork();
                switch (pid) {
                    case 0:
                        signal(SIGINT, SIG_DFL);
                        signal(SIGKILL, SIG_DFL);
                        signal(SIGTSTP, SIG_DFL);
                        //signal(SIGCHLD, SIG_DFL); Implementar nuestro handler para cuando muere un hijo
                        if (line->background) {
                            //Redirigir stdin, solución chapucera pero a que mola?
                            int input_fds = open("/dev/null", O_RDONLY);
                            dup2(input_fds, STDIN_FILENO);
                        }
                        if (line->redirect_output != NULL)
                            change_redirections(line, 1);
                        if (line->redirect_input != NULL)
                            change_redirections(line, 0);
                        if (line->redirect_error != NULL)
                            change_redirections(line, 2);
                        char *command = line->commands[0].filename; // Path absoluto
                        //printf("%s\n", command); Debug Filename

                        if (check_command(command)) {
                            execvp(command, line->commands[0].argv);
                            fprintf(stderr, "No se ha podido ejecutar el comando %s\n", command);
                        } else {
                            fprintf(stderr, "No se encuentra el comando %s\n", line->commands[0].argv[0]);
                            exit(-1);
                        }
                        break;

                    case ERROR:
                        fprintf(stderr, "Falló el fork");
                        exit(1);

                    default: // Padre
                        if (line->background) {
                            last_job++;
                            jobs_array[last_job].pid = pid;
                            strcpy(jobs_array[last_job].comando, line->commands[0].argv[0]);
                            jobs_array[last_job].line = line;
                            fprintf(stdout, "[%d] %d\n", last_job, pid);
                            break;
                        } else {
                            waitpid(pid, &exit_status, 0);
                            if (WIFEXITED(exit_status) != 0) {
                                if (WEXITSTATUS(exit_status) != 0) {
                                    fprintf(stdout, "El comando ha tenido un código de estado erróneo\n");
                                }
                            }
                            signal(SIGINT, SIG_IGN); //Ignorar señales en Padre
                            signal(SIGQUIT, SIG_IGN);
                            signal(SIGTSTP, SIG_IGN);
                            break;
                        }
                }
            }
        } else if (line->ncommands >= 2) {//executePipes
            signal(SIGCHLD, childHandler);
            executePipes(pipes_matrix, line, &last_job, jobs_array);
        }
        print_promt(exit_status);
    }
    free(buffer);
    return 0;
}

int check_command(const char *filename) {
    if (filename != NULL) {
        return True;
    } else {
        return False;
    }
}

void make_prompt() {
    char l_hostname[64];
    uid = getuid();
    gethostname(l_hostname, sizeof(l_hostname));
    strcpy(hostname, l_hostname);
    user = getenv("USER");
}

void my_cd(tline *line) {
    char *dir;
    int dir_status = ERROR;
    if (line->commands[0].argc > 2) {
        fprintf(stderr, "Más de dos argumentos\n");
    }

    if (line->commands[0].argc == 1) { // Solo me pasan cd, sin args
        dir = getenv("HOME");
        if (dir == NULL) {
            fprintf(stderr, "$HOME no tiene un valor\n");
        }
    } else {
        dir = line->commands[0].argv[1];
    }

    dir_status = chdir(dir);
    if (dir_status) { // Si la función devuelve algo que no es 0, falló
        fprintf(stderr, "No existe el directorio %s\n", dir);
    }
}

void print_promt(int exit_code) {
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
    if (exit_code != 0) {
        red();
        printf("status: %d ", exit_code);
        reset();
    }
    if (uid != 0)
        printf("$");
    else {
        red();
        printf("#");
        reset();
    }

    printf(" ");

}

void childHandler(int signal) {
    /* Hemos recibido la muerte de un hijo :'-( */
    pid_t temp;
    while ((temp = waitpid((pid_t) -1, NULL, WNOHANG)) > 0) {
        for (int i = 0; i <= last_job; ++i) {
            if (jobs_array[i].pid == temp) {
                printf("\n[%d] %d (%s) acabado\n", i, temp, jobs_array[i].comando);
                // Reordenar array
                for (int j = i; j <= last_job; ++j) {
                    jobs_array[j] = jobs_array[j + 1];
                }
                last_job--;
            }
        }
    }
}


void my_jobs() {
    char *command = (char *) malloc(1024 * sizeof(char));
    for (int i = 0; i <= last_job; ++i) {
        /*
        if (jobs_array[i].line->ncommands >= 2){ // Tenemos pipes
            for (int j = 0; j < jobs_array[i].line->ncommands; ++j) {
                strlcat(command, jobs_array[i].line->commands->argv[i], sizeof(command));
                strlcat(command, " ", sizeof(command));
            }
        } */
        printf("[%d] + Running %s PID: %d\n", i, jobs_array[i].comando, jobs_array[i].pid);
    }
    free(command);
}



