// aplicacao.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "comum.h"

int main(int argc, char *argv[]) {
    // Garante que o Kernel passou o ID do processo por argumento (0 para A1, 1 para A2, etc.)
    if (argc < 2) {
        fprintf(stderr, "[APLICACAO] Erro: ID do processo nao fornecido.\n");
        return 1;
    }
    
    int id = atoi(argv[1]);
    
    // 1. Abre a tabela de processos na memoria compartilhada nomeada [cite: 3]
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("[APLICACAO] Erro ao abrir memoria compartilhada");
        return 1;
    }
    
    // Mapeia a tabela no espaco de endereçamento deste processo executavel
    PCB *tabela = mmap(NULL, sizeof(PCB) * MAX_PROCESSOS, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (tabela == MAP_FAILED) {
        perror("[APLICACAO] Erro no mmap");
        return 1;
    }

    // O PID do Kernel (processo pai) é obtido via getppid()
    pid_t kernel_pid = getppid();
    printf("[A%d] Inicializado com sucesso (PID %d).\n", id + 1, getpid());

    // 2. Loop de execucao simulando as instrucoes ate MAX_PC [cite: 44]
    while (tabela[id].PC < MAX_PC) {
        
        // No corpo do loop deve haver um sleep(1) simulando o tempo da instrucao [cite: 45]
        sleep(1); 
        
        // Incrementa o contador de iteracoes PC (Troca de Contexto) 
        tabela[id].PC++;
        printf("   -> [A%d] Executando... Instrucao atual (PC = %d/%d)\n", id + 1, tabela[id].PC, MAX_PC);

        // 3. Define em qual tempo/instrucao sera executada a syscall [cite: 45]
        // Exemplo: Quando o PC chegar em 3, o processo solicita uma leitura (Read) 
        if (tabela[id].PC == 3) {
            printf("   !! [A%d] Solicitando Syscall(D1, R) no PC=%d.\n", id + 1, tabela[id].PC);
            
            // Os parametros da syscall tambem fazem parte do contexto salvo [cite: 50]
            tabela[id].syscall_type = 'R'; 
            
            // Envia o sinal SIGURG avisando o Kernel para tratar a Syscall
            kill(kernel_pid, SIGURG); 
            
            // Pequena pausa estrutural para dar tempo do Kernel receber o sinal
            // e suspender este processo com SIGSTOP antes que ele continue o loop 
            usleep(50000); 
        }
    }

    // 4. Ao sair do laço, define o estado como finalizado para o Kernel saber [cite: 49]
    tabela[id].state = TERMINATED;
    printf("[A%d] Execucao concluida com sucesso! Finalizando.\n", id + 1);
    
    return 0;
}