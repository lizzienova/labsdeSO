//Rafaela Bessa 2420043
//Lis Almeida 2421294

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {
    int n = 1;
    pid_t pid1, pid2, pid3;

    // cria filho 1
    pid1 = fork();
    if (pid1 < 0) {
        printf("erro no fork 1");
        exit(1);
    } else if (pid1 == 0) {
        // filho 1 soma
        for (int i = 0; i < 1000; i++) {
            n += 1;
        }
        printf("processo filho1, pid=%d, n=%d\n", getpid(), n);
        exit(0); // o filho 1 morre pra que nao continue o codigo
    }

    // so o pai existe aqui
    // cria filho 2
    pid2 = fork();
    if (pid2 < 0) {
        printf("erro no fork 2");
        exit(1);
    } else if (pid2 == 0) {
        // filho 2 soma
        for (int i = 0; i < 1000; i++) {
            n += 10;
        }
        printf("processo filho2, pid=%d, n=%d\n", getpid(), n);
        exit(0); // o filho 2 morre pra que nao continue o codigo
    }

    // so o pai existe aqui
    // cria filho 3
    pid3 = fork();
    if (pid3 < 0) {
        printf("erro no fork 3");
        exit(1);
    } else if (pid3 == 0) {
        // codigo do Filho 3
        for (int i = 0; i < 1000; i++) {
            n += 100;
        }
        printf("processo filho3, pid=%d, n=%d\n", getpid(), n);
        exit(0); // o filho 3 morre pra que nao continue o codigo
    }

    // pai espera os filhos terminarem
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);

    // print do pai so pra saber que ele terminou
    printf("processo pai (pid=%d) finalizando apos esperar todos os filhos.\n", getpid());

    return 0;
}