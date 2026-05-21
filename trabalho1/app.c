// App.c
// Lis Almeida || 2421294
// Rafaela Bessa || 2420043

// Trabalho1_LisAlmeida_2421294_RafaelaBessa_2420043
// Relatoriot1_LisAlmeida_2421294_RafaelaBessa_2420043
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
    printf("[A%d] Inicializado.\n", id + 1);

    while (tabela[id].PC < MAX_PC) {
        sleep(1); 
        
        tabela[id].PC++; 
        printf("  -> [A%d] Executando PC=%d\n", id + 1, tabela[id].PC);

        int co_syscall = 0;
        char operacao = '0';

        if (id == 0 && tabela[id].PC == 2) { 
            co_syscall = 1;
            operacao = 'R';
        } else if (id == 1 && tabela[id].PC == 3) { 
            co_syscall = 1;
            operacao = 'W';
        } else if (id == 2 && tabela[id].PC == 4) {
            co_syscall = 1;
            operacao = 'R';
        }

        // Se a condicao bater, dispara a Syscall
        if (co_syscall) {
            printf("  !! [A%d] Pedindo I/O (%c) no PC=%d\n", id + 1, operacao, tabela[id].PC);
            
            tabela[id].syscall_type = operacao; // Salva o parâmetro no contexto 
            kill(kernel_pid, SIGURG);                // Envia aviso de Syscall para o Kernel
            
            usleep(50000); 
        }
    }

    tabela[id].state = TERMINATED;
    printf("[A%d] Finalizado.\n", id + 1);
    
    return 0;
}