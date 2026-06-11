#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define V_SIZE 10000
#define NUM_THREADS 100

int vetor[V_SIZE];
sem_t sem_exclusao; // variável do semáforo POSIX

typedef struct {
    int id;
} ThreadData;

void* vetor_modifica_protecao(void* arg) {
    ThreadData* data = (ThreadData*) arg;
    int task_num = data->id;
    int modifica = (task_num % 2 != 0) ? -task_num : task_num;

    // DOWN: pede por acesso exclusivo na região com problema
    sem_wait(&sem_exclusao);

    for (int i = 0; i < V_SIZE; i++) {
        vetor[i] += modifica;
    }

    // UP: libera a região p/ a próxima thread
    sem_post(&sem_exclusao);

    free(data);
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];

    // inicializa o semáforo sem compartilhamento de processos
    if (sem_init(&sem_exclusao, 0, 1) != 0) {
        perror("Erro ao inicializar semáforo");
        return 1;
    }

    // inicializa o vetor global com 10
    for (int i = 0; i < V_SIZE; i++) {
        vetor[i] = 10;
    }

    // cria as threads
    for (int i = 1; i <= NUM_THREADS; i++) {
        ThreadData* data = malloc(sizeof(ThreadData));
        data->id = i;
        pthread_create(&threads[i-1], NULL, vetor_modifica_protecao, (void*)data);
    }

    // espera o fim de todas as threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    int esperado = 60; 
    int erros = 0;

    printf("Verificando consistência do vetor protegido...\n");
    for (int i = 0; i < V_SIZE; i++) {
        if (vetor[i] != esperado) {
            erros++;
        }
    }

    printf("Total de posições que foram corrompidas: %d de %d\n", erros, V_SIZE);

    // destrói o semáforo para liberar o sistema
    sem_destroy(&sem_exclusao);

    return 0;
}