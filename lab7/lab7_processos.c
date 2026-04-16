// lab7_processos.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/time.h>



#define N 10000
#define TRABALHADORES 10

int main() {
    int *vetor = mmap(NULL, N * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    for (int i = 0; i < N; i++) {
        vetor[i] = 5;
    }

    //processos filhos
    for (int i = 0; i < TRABALHADORES; i++) {
        if (fork() == 0) {
            // vai alterando o vetor por processo feito
            for (int j = 0; j < N; j++) {
                vetor[j] = vetor[j] * 2 + 2;
            }
            exit(0);
        }
    }

    //tempo
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    //espera
    for (int i = 0; i < TRABALHADORES; i++) {
        wait(NULL);
    }

    gettimeofday(&t2, NULL);

    long tempo = (t2.tv_sec - t1.tv_sec) * 1000000L +
                 (t2.tv_usec - t1.tv_usec);

    //aqui vê se são iguais
    int igual = 1;
    for (int i = 1; i < N; i++) {
        if (vetor[i] != vetor[0]) {
            igual = 0;
            break;
        }
    }

    printf("=== PROCESSOS ===\n");
    printf("Tempo: %ld microsegundos\n", tempo);
    printf("Valores iguais? %s\n", igual ? "SIM" : "NAO");
    printf("Valor final: %d\n", vetor[0]);

    return 0;
}