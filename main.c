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
#define JOBS "jobs"
#define G_USAGE "globalusage"
#define FG "fg"

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
job jobs_array[MAX_JOBS];
volatile pid_t pid_to_kill;
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

void controlChandler(int signal);

void my_fg(tline *line);

pid_t returnPidToKill();
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
    for (int i = 0; i < MAX_JOBS; ++i){
        jobs_array[i].eliminado = True;
    }

    make_prompt();
    tline *line; // Struct de parser.h
    char *buffer = (char *) malloc(1024 * (sizeof(char))); // Memoria dinámica porque why not
    print_promt(0);
    int exit_status = 0; //Se usa para comprobar salida de estado del hijo
    while (fgets(buffer, 1024, stdin)) {
        signal(SIGINT, SIG_IGN); //Hay que ignorar Ctrl+C y Ctrl+Z en la shell
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGCHLD, childHandler); // Si un hijo muere, tenemos que limpiar su estado en el array de jobs e imprimir
        line = tokenize(buffer);
        if (line == NULL) {
            continue;
        }
        if (line->ncommands == 1) {
            //Restablecemos las señales a las que están por defecto
            if (strcmp(line->commands[0].argv[0], CDCONST) == 0) { // Miramos si el comando que queremos es cd
                my_cd(line); // Aquí no hacemos hijo o no cambiamos de dir
            } else if (strcmp(line->commands[0].argv[0], EXIT) == 0) { // Salimos de la shell
                for (int i = 0; i < MAX_JOBS; ++i) {
                    if (jobs_array[i].eliminado == False){
                        kill(jobs_array[i].pid, SIGTERM);
                    }
                }
                exit(0);
            } else if (strcmp(line->commands[0].argv[0], JOBS) == 0) { // JOBS con fg y bg
                my_jobs();
            } else if (strcmp(line->commands[0].argv[0], FG) == 0){
                my_fg(line);
            }
            else if (strcmp(line->commands[0].argv[0], G_USAGE) == 0) { // Futuro Global Usage

            } else {
                //Creamos el hijo para ejecutar el comando
                pid_t pid;
                pid = fork();
                switch (pid) {
                    case 0:
                        setpgid(0,0);
                        signal(SIGINT, SIG_DFL);
                        signal(SIGKILL, SIG_DFL);
                        signal(SIGTSTP, SIG_DFL);
                        //signal(SIGCHLD, SIG_DFL); Implementar nuestro handler para cuando muere un hijo
                        if (line->background) {
                            //¿Redirigir stdin, solución chapucera, pero a que mola?
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
                            int counter = -1;
                            for (int i = 0; i < MAX_JOBS; ++i) {
                                if(jobs_array[i].eliminado == False){
                                    counter = i; // Última posición del array en la que tenemos un job
                                }
                            }
                            jobs_array[counter + 1].pid = pid;
                            jobs_array[counter + 1].pgid = pid;
                            buffer[strcspn(buffer, "\n")] = 0;
                            strcpy(jobs_array[counter + 1].comando, buffer);
                            jobs_array[counter + 1].eliminado = False;
                            fprintf(stdout, "[%d] %d\n", counter + 1, pid);
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
            executePipes(pipes_matrix, line, jobs_array, buffer);
        }
        print_promt(exit_status);
    }
    free(buffer);
    return 0;
}

void my_fg(tline *line) {

    int ultimo = -1;
    signal(SIGCHLD, childHandler);
    if (line->commands[0].argc == 1){
        for (int i = 0; i < MAX_JOBS; ++i) {
            if (jobs_array[i].eliminado == False){
                ultimo = i;
            }
        }
    } else if (line->commands[0].argc > 2){
        fprintf(stderr, "Bag fg usage\n");
        return;
    } else {
        int job = atoi(line->commands[0].argv[0]);
        if (jobs_array[job].eliminado == True){
            fprintf(stderr, "No existe un job con ese ID\n");
            return;
        } else {
            ultimo = job;
        }
    }
    pid_to_kill = ultimo;
    signal(SIGINT, controlChandler);
    printf("%d\n",pid_to_kill);
    waitpid(jobs_array[pid_to_kill].pid, NULL, 0);
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
        return;
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
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs_array[i].pid == temp) {
                jobs_array[i].eliminado = True;
                printf("\n[%d] %d (%s) acabado\n", i, temp, jobs_array[i].comando);
            }
        }
    }
}


void my_jobs() {
    char *command = (char *) malloc(1024 * sizeof(char));
    int index = -1;
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (jobs_array[i].eliminado == False) {
            printf("[%d] + Running (%s) PID: %d\n", ++index, jobs_array[i].comando, jobs_array[i].pid);
        }
    }
    free(command);
}

void controlChandler(int signal){
    killpg(jobs_array[pid_to_kill].pgid, SIGINT);
}



