//Rafaela Bessa 2420043
//Lis Almeida 2421294

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>

// alterar este valor para 100000 para testar a segunda parte do lab
#define TAMANHO 10000 
#define TRABALHADORES 10

int main() {
    printf("Parte 1:\n");
    
    // cria a área de memória para 1 número inteiro
    int shmid_n = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shmid_n < 0) { 
        printf("Erro no shmget do n"); 
        exit(1); 
    }

    // acopla e inicializa
    int *n = (int *) shmat(shmid_n, NULL, 0);
    *n = 1; 

    pid_t pid1, pid2, pid3;

    // filhos parte 1
    if ((pid1 = fork()) == 0) {
        for (int i = 0; i < 1000; i++) *n += 1;
        printf("Processo filho1, pid=%d, n=%d\n", getpid(), *n);
        exit(0);
    }
    
    if ((pid2 = fork()) == 0) {
        for (int i = 0; i < 1000; i++) *n += 10;
        printf("Processo filho2, pid=%d, n=%d\n", getpid(), *n);
        exit(0);
    }
    
    if ((pid3 = fork()) == 0) {
        for (int i = 0; i < 1000; i++) *n += 100;
        printf("Processo filho3, pid=%d, n=%d\n", getpid(), *n);
        exit(0);
    }

    // pai espera os 3 filhos
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);

    printf("Processo pai finalizado. Valor final de n = %d\n", *n);

    // desacopla e destrói a memória do 'n'
    shmdt(n);
    shmctl(shmid_n, IPC_RMID, NULL);



    printf("\nParte 2: (%d posicoes) \n", TAMANHO);

    // cria a área de memória para o vetor
    int shmid_vetor = shmget(IPC_PRIVATE, TAMANHO * sizeof(int), IPC_CREAT | 0666);
    if (shmid_vetor < 0) { 
        printf("Erro no shmget do vetor"); 
        exit(1); 
    }

    // acopla e inicializa o vetor com 1
    int *vetor = (int *) shmat(shmid_vetor, NULL, 0);
    for (int i = 0; i < TAMANHO; i++) {
        vetor[i] = 1;
    }

    pid_t pids[TRABALHADORES];

    // filhos parte 2
    for (int w = 0; w < TRABALHADORES; w++) {
        pids[w] = fork();
        
        if (pids[w] == 0) { // código do filho trabalhador
            for (int i = 0; i < TAMANHO; i++) {
                vetor[i] = vetor[i] * 2 + 2;
            }
            exit(0); // trabalhador morre após varrer o vetor
        }
    }

    // pai espera os 10
    for (int w = 0; w < TRABALHADORES; w++) {
        waitpid(pids[w], NULL, 0);
    }

    int primeiro_valor = vetor[0];
    int todos_iguais = 1;
    int posicoes_erradas = 0;

    for (int i = 1; i < TAMANHO; i++) {
        if (vetor[i] != primeiro_valor) {
            todos_iguais = 0;
            posicoes_erradas++;
        }
    }

    if (todos_iguais) {
        printf("SUCESSO\n");
    } else {
        printf("FALHA: Encontradas %d posicoes divergentes\n", posicoes_erradas);
    }

    // desacopla e destrói a memória do vetor
    shmdt(vetor);
    shmctl(shmid_vetor, IPC_RMID, NULL);

    return 0;
}