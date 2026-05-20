// Intercontroller.c
// Lis Almeida || 2421294
// Rafaela Bessa || 2420043

// Trabalho1_LisAlmeida_2421294_RafaelaBessa_2420043
// Relatoriot1_LisAlmeida_2421294_RafaelaBessa_2420043
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/types.h>
#include "comum.h"

int main() {
    // Pega o PID do kernel (processo pai)
    pid_t kernel_pid = getppid();

    // Conecta na fila de mensagens IPC
    int msgid = msgget(MSG_KEY, 0666);
    if (msgid == -1) {
        perror("[INTERCONTROLLER] Erro ao obter a fila de mensagens");
        exit(1);
    }

    // Array para gerenciar os timers de I/O
    int timers_io[MAX_PROCESSOS] = {0}; 

    printf("[HW] Inicializado.\n");

    // Loop principal do hardware
    while (1) {
        // Espera 1 segundo (clock)
        sleep(1);
        
        // Manda IRQ0 pro Kernel (fim do timeslice)
        kill(kernel_pid, SIGUSR1); 

        // Checa se tem novo pedido de I/O na fila
        MensagemIPC msg;
        while (msgrcv(msgid, &msg, sizeof(MensagemIPC) - sizeof(long), TYPE_START_IO, IPC_NOWAIT) != -1) {
            printf("[HW] Novo pedido de I/O (Slot %d)\n", msg.id_processo);
            
            // Acha um slot livre e seta o timer pra 3s
            for (int i = 0; i < MAX_PROCESSOS; i++) {
                if (timers_io[i] == 0) {
                    timers_io[i] = 3; 
                    break;
                }
            }
        }

        // Atualiza os timers ativos
        for (int i = 0; i < MAX_PROCESSOS; i++) {
            if (timers_io[i] > 0) {
                timers_io[i]--;
                
                // Se o tempo de E/S esgotou
                if (timers_io[i] == 0) {
                    printf("[HW] I/O concluido (IRQ1)\n");
                    
                    // Dispara IRQ1 pro Kernel
                    kill(kernel_pid, SIGUSR2);
                    
                    // Evita encavalamento de sinais
                    usleep(50000); 
                }
            }
        }
    }

    return 0;
}