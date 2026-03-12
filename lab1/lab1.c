#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

// lab1_rafaela_bessa_lis_villanova
//matriculas: 2420043 || 2421294

int main() {
    int n = 1;
    pid_t pid_filho, pid_neto;

    pid_filho = fork();
   
    // processo do pai
    if (pid_filho < 0) {
        // deu errado
        printf("erro ao criar filho\n");
        exit(1);
    }
    else if (pid_filho > 0) {
        // pai soma 1 a n
        for (int i = 0; i < 1000; i++) {
            n += 1;
            printf("processo pai, pid = %d, n = %d\n", getpid(), n);
        }
       
        waitpid(pid_filho, NULL, 0); // espera o filho
    }
    else {
        // processo do filho
        pid_neto = fork();

        if (pid_neto < 0) {
            printf("erro ao criar neto\n");
            exit(1);
        }
        else if (pid_neto > 0) {
            // filho soma 10 a n
            for (int i = 0; i < 1000; i++) {
                n += 10;
                printf("processo filho, pid = %d, n = %d\n", getpid(), n);
            }
           waitpid(pid_neto, NULL, 0);; // espera o neto
        }
        else {
            // processo do neto
            // neto soma 100 a n
            for (int i = 0; i < 1000; i++) {
                n += 100;
                printf("processo neto, pid = %d, n = %d\n", getpid(), n);
            }
            exit(0); // neto termina
        }
    }

    return 0;
}