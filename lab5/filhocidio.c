#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int delay;

// executado se o filho morrer antes do pai acordar
void childhandler(int signo) {
    int status;
    pid_t pid = wait(&status);
    printf("Child %d terminated within %d seconds com estado %d\n", pid, delay, status);
    exit(0);
}

int main(int argc, char *argv[]) {
    pid_t pid;

    if (argc < 2) {
        printf("Uso: ./filhocidio <tempo_espera> [comando_opcional]\n");
        exit(1);
    }

    signal(SIGCHLD, childhandler);

    if ((pid = fork()) < 0) {
        fprintf(stderr, "Erro ao criar filho\n");
        exit(-1);
    }

    if (pid == 0) { /* Código do Filho */
        
        // DESCOMENTAR A OPÇÃO QUE QUERO TESTAR

        // a) loop eterno
        // for(;;); 

        // b) dorme 3 segundos
        // sleep(3); exit(0); 

        // c e d) executa programa externo
        if (argc >= 3) {
           execvp(argv[2], &argv[2]);
        } else {
            for(;;); 
        }

    } else { /* código do Pai */
        sscanf(argv[1], "%d", &delay);
        sleep(delay);
        if (kill(pid, 0) == 0) {
            printf("Program exceeded limit of %d seconds! Matando o filho...\n", delay);
            kill(pid, SIGKILL);
        }
        return 0;
    }
}