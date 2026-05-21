#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "comum.h"

int main(int argc, char *argv[]) {
    int id = atoi(argv[1]);
    
    // ATTACH DA MEMÓRIA COMPARTILHADA
    // Repare que NÃO tem a flag O_CREAT. A aplicação assume que o Kernel já alocou a RAM.
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    PCB *tabela = mmap(NULL, sizeof(PCB) * MAX_PROCESSOS, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    pid_t kernel_pid = getppid(); // Identifica o Kernel para poder enviar Syscalls.
    printf("[A%d] Inicializado.\n", id + 1);

    // CICLO DE INSTRUÇÕES (A CPU rodando o processo)
    while (tabela[id].PC < MAX_PC) {
        
        // Simulação de "Burst de CPU" (Uma instrução matemática pesada)
        sleep(1); 
        
        // Atualiza o Program Counter. Como o mmap está em MAP_SHARED,
        // o Kernel enxerga essa alteração instantaneamente.
        tabela[id].PC++; 
        printf("  -> [A%d] Executando PC=%d\n", id + 1, tabela[id].PC);

        int condicao_syscall = 0;
        char tipo_operacao = '0';

        // REGRAS DE I/O HARDCODED (Simulando uma arquitetura real)
        // Em um SO real, isso ocorreria sempre que a thread batesse num comando "printf" ou "fread".
        if (id == 0 && tabela[id].PC == 2) { 
            condicao_syscall = 1; tipo_operacao = 'R';
        } else if (id == 1 && tabela[id].PC == 3) { 
            condicao_syscall = 1; tipo_operacao = 'W';
        } else if (id == 2 && tabela[id].PC == 4) {
            condicao_syscall = 1; tipo_operacao = 'R';
        }

        // DISPARO DA SYSCALL
        if (condicao_syscall) {
            printf("  !! [A%d] Pedindo I/O (%c) no PC=%d\n", id + 1, tipo_operacao, tabela[id].PC);
            
            // 1. Passagem de Parâmetro: Salva a intenção na memória antes de avisar o Kernel.
            tabela[id].syscall_type = tipo_operacao; 
            
            // 2. O Disparo (Software Interrupt): Envia o sinal crítico pro Kernel assumir o controle.
            kill(kernel_pid, SIGURG); 
            
            // 3. SINCRONISMO (A Proteção contra Fugas)
            // Se essa linha não existisse, a aplicação poderia voltar pro começo do 'while',
            // incrementar o PC para 3 e causar um bug lógico na simulação ANTES do Kernel 
            // processar a interrupção. O usleep dá o tempo pro Kernel capturar a flag SIGURG 
            // e aplicar o SIGSTOP nesta aplicação.
            usleep(50000); 
        }
    }

    // FINALIZAÇÃO DE PROCESSO
    tabela[id].state = TERMINATED;
    printf("[A%d] Finalizado.\n", id + 1);
    
    return 0;
}