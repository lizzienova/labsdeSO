// intercontroller.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/types.h>
#include "comum.h"

int main() {
    // Como esse processo é criado via fork/exec pelo Kernel, 
    // o PID do Kernel (processo pai) é obtido diretamente por getppid()
    pid_t kernel_pid = getppid();

    // Conecta-se à fila de mensagens IPC criada pelo Kernel
    int msgid = msgget(MSG_KEY, 0666);
    if (msgid == -1) {
        perror("[INTERCONTROLLER] Erro ao obter a fila de mensagens");
        exit(1);
    }

    // Array para gerenciar múltiplos temporizadores de I/O de forma concorrente [cite: 41]
    int timers_io[MAX_PROCESSOS] = {0}; 

    printf("[INTERCONTROLLER] Hardware Inicializado (PID %d). Monitorando Kernel (PID %d)...\n", getpid(), kernel_pid);

    // Loop infinito simulando o hardware em tempo real [cite: 25, 40]
    while (1) {
        // Base de tempo do clock do sistema (1 segundo) [cite: 21, 40]
        sleep(1);
        
        // --- GERAÇÃO DO IRQ0 (Fim de Timeslice) ---
        // Envia SIGUSR1 para o Kernel indicar a interrupção de relógio [cite: 40]
        kill(kernel_pid, SIGUSR1); 

        // --- VERIFICAÇÃO DE NOVOS PEDIDOS DE I/O ---
        // Checa se o Kernel enviou alguma mensagem na fila avisando que um app bloqueou
        MensagemIPC msg;
        while (msgrcv(msgid, &msg, sizeof(MensagemIPC) - sizeof(long), TYPE_START_IO, IPC_NOWAIT) != -1) {
            printf("[INTERCONTROLLER] Hardware: Novo pedido de I/O detectado para o processo no slot %d.\n", msg.id_processo);
            
            // Encontra um slot de temporizador livre no hardware para começar a contagem de 3s [cite: 41]
            for (int i = 0; i < MAX_PROCESSOS; i++) {
                if (timers_io[i] == 0) {
                    timers_io[i] = 3; // Define os 3 segundos regulamentares do dispositivo D1 [cite: 38, 41]
                    break;
                }
            }
        }

        // --- ATUALIZAÇÃO DOS TIMERS DE I/O ATIVOS ---
        // Decrementa o tempo dos processos que estão realizando operação no dispositivo D1
        for (int i = 0; i < MAX_PROCESSOS; i++) {
            if (timers_io[i] > 0) {
                timers_io[i]--;
                
                // Se o tempo esgotou (passaram-se os 3 segundos) [cite: 38, 41]
                if (timers_io[i] == 0) {
                    printf("[INTERCONTROLLER] Hardware: Operacao de E/S concluida no dispositivo D1. Disparando IRQ1...\n");
                    // Envia SIGUSR2 para o Kernel correspondente ao IRQ1 [cite: 41]
                    kill(kernel_pid, SIGUSR2);
                }
            }
        }
    }

    return 0;
}