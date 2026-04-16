#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

//Rafaela Bessa 2420043
//Lis Almeida 2421294

#define N 10000
#define TRABALHADORES 10

int vetor[N];

void *trabalhador(void *arg) {
    struct timeval t1, t2;
    gettimeofday(&t1, NULL); // inicia o tempo DENTRO da thread

    for (int i = 0; i < N; i++) {
        vetor[i] = vetor[i] * 2 + 2;
    }

    gettimeofday(&t2, NULL); // para o tempo
    long tempo = (t2.tv_sec - t1.tv_sec) * 1000000L + (t2.tv_usec - t1.tv_usec);
    printf("Thread executou em: %ld microsegundos\n", tempo);

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

    // espera as threads acabarem
    for (int i = 0; i < TRABALHADORES; i++) {
        pthread_join(threads[i], NULL);
    }

    // vendo se é igual
    int igual = 1;
    for (int i = 1; i < N; i++) {
        if (vetor[i] != vetor) {
            igual = 0;
            break;
        }
    }

    printf("\nRESULTADO THREADS\n");
    printf("valores iguais? %s\n", igual ? "SIM" : "NAO");
    printf("valor da posicao: %d\n", vetor);

    return 0;
}