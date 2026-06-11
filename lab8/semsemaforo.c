#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define V_TAM 10000
#define NUM_THREADS 100

int vetor[V_TAM];

// estrutura p/ passar o ID/num da tarefa p thread
typedef struct {
    int id;
} ThreadData;

void* vetor_modifica(void* arg) {
    ThreadData* data = (ThreadData*) arg;
    int task_num = data->id;
    
    // impar subtrai, par soma
    int modificador = (task_num % 2 != 0) ? -task_num : task_num;

    for (int i = 0; i < V_TAM; i++) {
        // região sem proteção - pode gerar concorrencia
        vetor[i] += modificador;
    }

    free(data);
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];

    // inicializa o vetor global c/ 10
    for (int i = 0; i < V_TAM; i++) {
        vetor[i] = 10;
    }

    // cria as 100 threads concorrentes
    for (int i = 1; i <= NUM_THREADS; i++) {
        ThreadData* data = malloc(sizeof(ThreadData));
        data->id = i;
        if (pthread_create(&threads[i-1], NULL, vetor_modifica, (void*)data) != 0) {
            perror("Erro na criação da thread");
            return 1;
        }
    }

    // Espera todas as threads finalizarem
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // valor esperado p/ cada posição: 10-1+2-3+4...+100=60
    int esperado = 60; 
    int erros = 0;

    printf("Verificando consistência do vetor...\n");
    for (int i = 0; i < V_TAM; i++) {
        if (vetor[i] != esperado) {
            erros++;
            if (erros <= 10) { // mostra apenas as primeiras inconsistências
                printf("Inconsistência na posição [%d]: O esperado era %d e o obtido foi %d\n", i, esperado, vetor[i]);
            }
        }
    }

    printf("\nTotal de posições corrompidas pela concorrência: %d de %d\n", erros, V_TAM);
    return 0;
}