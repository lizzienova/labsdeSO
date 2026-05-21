// intercontroller.c
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
#include "definicoes.h"

int main() {
    pid_t pid_so = getppid();

    // conecta na fila de msg pra ouvir o kernel
    int id_caixa_msg = msgget(MSG_KEY, 0666);
    if (id_caixa_msg == -1) {
        perror("Intercontroller -> Erro ao obter a fila de mensagens\n");
        exit(1);
    }

    // array pra guardar os 3 seg de cada processo q ta no disco
    int cronometros_io [ MAX_PROCESSOS ]  = {0}; 

    printf("[HW] Inicializado.\n");

    // loop eterno do hardware
    while (1) {
        sleep(1); // batida do relogio
        
        kill(pid_so, SIGUSR1); // manda o irq0 (time slice)

        AvisoHardware pacote_recebido;
        // checa se chegou msg nova de alguem pedindo io (nao bloqueante)
        while (msgrcv(id_caixa_msg, &pacote_recebido, sizeof(AvisoHardware) - sizeof(long), TYPE_START_IO, IPC_NOWAIT) != -1) {
            printf("[HW] Novo pedido de I/O (Slot %d)\n", pacote_recebido.id_do_app);
            
            // acha um espaco vazio nos cronometros e seta 3 segundos
            for (int k = 0; k < MAX_PROCESSOS; k++) {
                if (cronometros_io [ k ]  == 0) {
                    cronometros_io [ k ]  = 3; 
                    break;
                }
            }
        }

        // diminui o tempo de todo mundo que ta processando io
        for (int c = 0; c < MAX_PROCESSOS; c++) {
            if (cronometros_io [ c ]  > 0) {
                cronometros_io [ c ] --;
                
                // se zerou o tempo, avisa q acabou
                if (cronometros_io [ c ]  == 0) {
                    printf("[HW] I/O concluido (IRQ1)\n");
                    
                    kill(pid_so, SIGUSR2); // manda o irq1 pro kernel
                    
                    usleep(50000); // delay de seguranca
                }
            }
        }
    }

    return 0;
}