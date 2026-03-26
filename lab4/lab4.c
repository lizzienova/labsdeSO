//Rafaela Bessa 2420043
//Lis Almeida 2421294

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>

// Altere este valor para 100000 para testar a segunda exigência do Lab
#define TAMANHO 10000 
#define TRABALHADORES 10

int main() {
    printf("PARTE 1: Variavel 'n' Compartilhada \n");
    
    // 1. Cria a área de memória para 1 número inteiro
    int shmid_n = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shmid_n < 0) { perror("Erro no shmget do n"); exit(1); }

    // Acopla e inicializa
    int *n = (int *) shmat(shmid_n, NULL, 0);
    *n = 1; 

    pid_t pid1, pid2, pid3;

    // --- FILHOS DA PARTE 1 ---
    if ((pid1 = fork()) == 0) {
        for (int i = 0; i < 1000; i++) *n += 1;
        printf("processo filho1, pid=%d, n=%d\n", getpid(), *n);
        exit(0);
    }
    
    if ((pid2 = fork()) == 0) {
        for (int i = 0; i < 1000; i++) *n += 10;
        printf("processo filho2, pid=%d, n=%d\n", getpid(), *n);
        exit(0);
    }
    
    if ((pid3 = fork()) == 0) {
        for (int i = 0; i < 1000; i++) *n += 100;
        printf("processo filho3, pid=%d, n=%d\n", getpid(), *n);
        exit(0);
    }

    // Pai espera os 3 filhos da Parte 1
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);

    printf("Processo pai finalizando Parte 1. Valor final de n = %d\n", *n);

    // Desacopla e destrói a memória do 'n'
    shmdt(n);
    shmctl(shmid_n, IPC_RMID, NULL);



    printf("\n========== PARTE 2: Vetor Compartilhado (%d posicoes) ==========\n", TAMANHO);

    // 1. Cria a área de memória para o vetor
    int shmid_vetor = shmget(IPC_PRIVATE, TAMANHO * sizeof(int), IPC_CREAT | 0666);
    if (shmid_vetor < 0) { perror("Erro no shmget do vetor"); exit(1); }

    // Acopla e inicializa o vetor com 1
    int *vetor = (int *) shmat(shmid_vetor, NULL, 0);
    for (int i = 0; i < TAMANHO; i++) {
        vetor[i] = 1;
    }

    pid_t pids[TRABALHADORES];

    // --- FILHOS DA PARTE 2 (Trabalhadores) ---
    for (int w = 0; w < TRABALHADORES; w++) {
        pids[w] = fork();
        
        if (pids[w] == 0) { // Código do Filho Trabalhador
            for (int i = 0; i < TAMANHO; i++) {
                vetor[i] = vetor[i] * 2 + 2;
            }
            exit(0); // Trabalhador morre após varrer o vetor
        }
    }

    // Pai espera os 10 trabalhadores terminarem
    for (int w = 0; w < TRABALHADORES; w++) {
        waitpid(pids[w], NULL, 0);
    }

    // --- VALIDAÇÃO AUTOMÁTICA ---
    int primeiro_valor = vetor;
    int todos_iguais = 1;
    int posicoes_erradas = 0;

    for (int i = 1; i < TAMANHO; i++) {
        if (vetor[i] != primeiro_valor) {
            todos_iguais = 0;
            posicoes_erradas++;
        }
    }

    if (todos_iguais) {
        printf("SUCESSO: Todas as posicoes tem o mesmo valor (%d).\n", primeiro_valor);
    } else {
        printf("FALHA DE CONSISTENCIA: Encontradas %d posicoes divergentes!\n", posicoes_erradas);
        printf("Isso comprova a ocorrencia de Condicao de Corrida (Race Condition).\n");
    }

    // Desacopla e destrói a memória do vetor
    shmdt(vetor);
    shmctl(shmid_vetor, IPC_RMID, NULL);

    return 0;
}