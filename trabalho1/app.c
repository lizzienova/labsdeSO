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
    // Agora o app espera o ID e a ordem se deve fazer I/O ou não
    if (argc < 3) {
        fprintf(stderr, "[APP] Erro: Argumentos insuficientes.\n");
        return 1;
    }
    
    int id = atoi(argv[1]);
    int faz_io = atoi(argv[2]); // 0 = Não faz I/O | 1 = Faz I/O
    
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

        // LÓGICA CORINGA: O Kernel mandou fazer I/O? Se sim, faz no PC=3.
        if (faz_io == 1 && tabela[id].PC == 3) { 
            co_syscall = 1;
            operacao = (id % 2 == 0) ? 'R' : 'W'; 
        }

        // Se a condicao bater, dispara a Syscall
        if (co_syscall) {
            printf("  !! [A%d] Pedindo I/O (%c) no PC=%d\n", id + 1, operacao, tabela[id].PC);
            
            tabela[id].syscall_type = operacao; 
            kill(kernel_pid, SIGURG);                
            
            usleep(50000); 
        }
    }

    tabela[id].state = TERMINATED;
    printf("[A%d] Finalizado.\n", id + 1);
    
    return 0;
}