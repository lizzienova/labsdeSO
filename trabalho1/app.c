// app.c
// Lis Almeida || 2421294
// Rafaela Bessa || 2420043

// trabalho1_LisAlmeida_2421294_RafaelaBessa_2420043
// relatoriot1_LisAlmeida_2421294_RafaelaBessa_2420043
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
        fprintf(stderr, "[APP] Erro: ID do processo nao fornecido.\n");
        return 1;
    }
    
    int id = atoi(argv[1]);
    
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("[APP] Erro ao abrir memoria compartilhada");
        return 1;
    }
    
    PCB *tabela = mmap(NULL, sizeof(PCB) * MAX_PROCESSOS, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (tabela == MAP_FAILED) {
        perror("[APP] Erro no mmap");
        return 1;
    }

    pid_t kernel_pid = getppid();
    printf("[A%d] Inicializado com sucesso (PID %d).\n", id + 1, getpid());

    while (tabela[id].PC < MAX_PC) {
        sleep(1); 
        
        tabela[id].PC++; 
        printf("   -> [A%d] Executando... Instrucao atual (PC = %d/%d)\n", id + 1, tabela[id].PC, MAX_PC);

        int condicao_syscall = 0;
        char tipo_operacao = '0';

        if (id == 0 && tabela[id].PC == 2) { 
            condicao_syscall = 1;
            tipo_operacao = 'R';
        } else if (id == 1 && tabela[id].PC == 3) { 
            condicao_syscall = 1;
            tipo_operacao = 'W';
        } else if (id == 2 && tabela[id].PC == 4) {
            condicao_syscall = 1;
            tipo_operacao = 'R';
        }

        // Se a condiçãobater, dispara a Syscall
        if (condicao_syscall) {
            printf("   !! [A%d] Executando Syscall(D1, %c) no PC=%d. Solicitando E/S...\n", id + 1, tipo_operacao, tabela[id].PC);
            
            tabela[id].syscall_type = tipo_operacao; // Salva o parâmetro no contexto 
            kill(kernel_pid, SIGURG);                // Envia aviso de Syscall para o Kernel
            
            usleep(50000); 
        }
    }

    tabela[id].state = TERMINATED;
    printf("[A%d] Execucao concluida com sucesso! Finalizando.\n", id + 1);
    
    return 0;
}