// lab7_threads.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define N 10000
#define TRABALHADORES 10

int vetor[N];

void *trabalhador(void *arg) {
    for (int i = 0; i < N; i++) {
        vetor[i] = vetor[i] * 2 + 2;
    }
    return NULL;
}

int main() {
    pthread_t threads[TRABALHADORES];
    for (int i = 0; i < N; i++) {
        vetor[i] = 5;
    }

    for (int i = 0; i < TRABALHADORES; i++) {
        pthread_create(&threads[i], NULL, trabalhador, NULL);
    }

    //começa a analisar tempo
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    // espera as threads acabarem
    for (int i = 0; i < TRABALHADORES; i++) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&t2, NULL);

    long tempo = (t2.tv_sec - t1.tv_sec) * 1000000L +
                 (t2.tv_usec - t1.tv_usec);

    // vendo se é igual
    int igual = 1;
    for (int i = 1; i < N; i++) {
        if (vetor[i] != vetor[0]) {
            igual = 0;
            break;
        }
    }

    printf("=== THREADS ===\n");
    printf("Tempo: %ld microsegundos\n", tempo);
    printf("Valores iguais? %s\n", igual ? "SIM" : "NAO");
    printf("Valor final: %d\n", vetor[0]);

    return 0;
}