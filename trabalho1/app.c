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
    if (argc < 2) {
        fprintf(stderr, "[APLICACAO] Erro: ID do processo nao fornecido.\n");
        return 1;
    }
    
    int id = atoi(argv[1]);
    
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("[APLICACAO] Erro ao abrir memoria compartilhada");
        return 1;
    }
    
    PCB *tabela = mmap(NULL, sizeof(PCB) * MAX_PROCESSOS, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (tabela == MAP_FAILED) {
        perror("[APLICACAO] Erro no mmap");
        return 1;
    }

    pid_t kernel_pid = getppid();
    printf("[A%d] Inicializado com sucesso (PID %d).\n", id + 1, getpid());

    // Loop de execução simulando as instruções até MAX_PC [cite: 44]
    while (tabela[id].PC < MAX_PC) {
        sleep(1); // No corpo do loop deverá haver um sleep(1) [cite: 45]
        
        tabela[id].PC++; // Incrementa contador de iterações (PC) [cite: 44]
        printf("   -> [A%d] Executando... Instrucao atual (PC = %d/%d)\n", id + 1, tabela[id].PC, MAX_PC);

        // --- CENÁRIO DE TESTES DINÂMICO ---
        // Definimos tempos diferentes para as syscalls dependendo do processo [cite: 45]
        int condicao_syscall = 0;
        char tipo_operacao = '0';

        if (id == 0 && tabela[id].PC == 2) { 
            // A1 pede Leitura (Read) no PC=2 [cite: 13]
            condicao_syscall = 1;
            tipo_operacao = 'R';
        } else if (id == 1 && tabela[id].PC == 3) { 
            // A2 pede Escrita (Write) no PC=3 [cite: 15]
            condicao_syscall = 1;
            tipo_operacao = 'W';
        } else if (id == 2 && tabela[id].PC == 4) {
            // A3 pede Leitura (Read) no PC=4
            condicao_syscall = 1;
            tipo_operacao = 'R';
        }

        // Se a condição de teste bater, dispara a Syscall [cite: 34]
        if (condicao_syscall) {
            printf("   !! [A%d] Executando Syscall(D1, %c) no PC=%d. Solicitando E/S...\n", id + 1, tipo_operacao, tabela[id].PC);
            
            tabela[id].syscall_type = tipo_operacao; // Salva parâmetro no contexto [cite: 50]
            kill(kernel_pid, SIGURG);                // Envia aviso de Syscall para o Kernel
            
            usleep(50000); // Aguarda o Kernel processar e enviar o SIGSTOP [cite: 35]
        }
    }

    tabela[id].state = TERMINATED;
    printf("[A%d] Execucao concluida com sucesso! Finalizando.\n", id + 1);
    
    return 0;
}