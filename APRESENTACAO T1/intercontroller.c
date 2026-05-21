// intercontroller.c
// Lis Almeida || 2421294
// Rafaela Bessa || 2420043
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/types.h>
#include "definicoes.h"

int main() {
    // guarda o pid do kernel pra enviar os irqs
    pid_t pid_so = getppid();

    // abre a fila de mensagens
    int id_caixa_msg = msgget(MSG_KEY, 0666);
    if (id_caixa_msg == -1) {
        perror("Intercontroller -> Erro ao obter a fila de mensagens\n");
        exit(1);
    }

    // array pra controlar o tempo de io de multiplos processos ao mesmo tempo
    int cronometros_io[MAX_PROCESSOS] = {0}; 

    printf("[HW] Inicializado.\n");

    // loop infinito simulando o hardware
    while (1) {
        // clock de 1 segundo
        sleep(1); 
        
        // manda o sinal de interrupcao de relogio (irq0) pro kernel
        kill(pid_so, SIGUSR1); 

        AvisoHardware pacote_recebido;
        // le a fila de mensagens no modo nao bloqueante (ipc_nowait)
        while (msgrcv(id_caixa_msg, &pacote_recebido, sizeof(AvisoHardware) - sizeof(long), TYPE_START_IO, IPC_NOWAIT) != -1) {
            printf("[HW] Novo pedido de I/O (Slot %d)\n", pacote_recebido.id_do_app);
            
            // acha um cronometro livre e coloca 3 segundos
            for (int k = 0; k < MAX_PROCESSOS; k++) {
                if (cronometros_io[k] == 0) {
                    cronometros_io[k] = 3; 
                    break;
                }
            }
        }

        // atualiza todos os cronometros que estao ativos
        for (int c = 0; c < MAX_PROCESSOS; c++) {
            if (cronometros_io[c] > 0) {
                cronometros_io[c]--; // decrementa 1 segundo
                
                // se o io terminou (zerou o tempo)
                if (cronometros_io[c] == 0) {
                    printf("[HW] I/O concluido (IRQ1)\n");
                    
                    // manda o sinal de fim de io (irq1) pro kernel
                    kill(pid_so, SIGUSR2); 
                    
                    // pausa de seguranca pra evitar perda de sinal concorrente
                    usleep(50000); 
                }
            }
        }
    }

    return 0;
}