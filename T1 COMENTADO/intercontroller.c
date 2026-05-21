#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/types.h>
#include "comum.h"

int main() {
    pid_t kernel_pid = getppid();

    // Attach na fila de mensagens IPC. Apenas permissão 0666, sem O_CREAT.
    int msgid = msgget(MSG_KEY, 0666);

    // Controladora de Discos Simulada (Suporta até 3 I/Os Concorrentes)
    int timers_io[MAX_PROCESSOS] = {0}; 

    printf("[HW] Inicializado.\n");

    // Loop Infinito do Ciclo de Máquina
    while (1) {
        
        // -------------------------------------------------------------
        // TIMER DA PLACA MÃE (Timeslice para a CPU)
        // -------------------------------------------------------------
        sleep(1);
        // Interrupção de Hardware 0 (IRQ0) mandando o Kernel trocar de processo.
        kill(kernel_pid, SIGUSR1); 

        // -------------------------------------------------------------
        // MONITORAMENTO DE E/S (Input/Output Bus)
        // -------------------------------------------------------------
        MensagemIPC msg;
        
        // O Kernel enviou requisições?
        // IPC_NOWAIT = Leitura Não-Bloqueante. Se não tiver nada, o 'msgrcv' retorna -1 
        // imediatamente em vez de travar o hardware esperando dados.
        while (msgrcv(msgid, &msg, sizeof(MensagemIPC) - sizeof(long), TYPE_START_IO, IPC_NOWAIT) != -1) {
            printf("[HW] Novo pedido de I/O (Slot %d)\n", msg.id_processo);
            
            // Aloca um canal de disco para o processo que pediu I/O.
            for (int i = 0; i < MAX_PROCESSOS; i++) {
                if (timers_io[i] == 0) { 
                    timers_io[i] = 3; // Define latência do disco: 3 segundos lógicos.
                    break; 
                }
            }
        }

        // -------------------------------------------------------------
        // PROCESSAMENTO DE E/S PENDENTES
        // -------------------------------------------------------------
        for (int i = 0; i < MAX_PROCESSOS; i++) {
            if (timers_io[i] > 0) {
                timers_io[i]--; // Disco leu dados por 1 segundo lógico.
                
                // Transferência DMA/Disco Concluída!
                if (timers_io[i] == 0) {
                    printf("[HW] I/O concluido (IRQ1)\n");
                    
                    // Interrupção de Hardware 1 (IRQ1) avisando o Kernel do término.
                    kill(kernel_pid, SIGUSR2); 
                    
                    // PROTEÇÃO CONTRA SIGNAL COALESCING NO LINUX
                    // Sinais padrão (SIGUSR) não criam filas no Unix. Se o hardware disparar
                    // dois sinais idênticos em menos de 1 milissegundo, o Linux funde os dois
                    // em um só para economizar processamento. O usleep(50000) injeta um atraso 
                    // de 0,05 segundos, garantindo tempo hábil para o tratador de sinal do Kernel
                    // rodar e liberar a fila antes do próximo sinal ser disparado.
                    usleep(50000); 
                }
            }
        }
    }

    return 0;
}