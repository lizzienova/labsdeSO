// intercontroller.c
// Lis Almeida || 2421294
// Rafaela Bessa || 2420043
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/types.h>
#include "definicoes.h"

int main() {
    pid_t pid_so = getppid();
    int id_caixa_msg = msgget(MSG_KEY, 0666);
    
    
    // o hardware tambem se conecta a memoria pra ler o relogio global
    int fd_memoria = shm_open(SHM_NAME, O_RDWR, 0666);
    MemoriaCompartilhada *mural = mmap(NULL, sizeof(MemoriaCompartilhada), PROT_READ | PROT_WRITE, MAP_SHARED, fd_memoria, 0);

    int cronometros_io[MAX_PROCESSOS] = {0}; 

    printf("[HW] Controlador de Interrupcoes Iniciado.\n");

    while (1) {
        
        // o hardware agora dita o ritmo. a cada 1s real, ele aumenta o tempo global
        sleep(1); 
        mural->tempo_global++; 
        
        for (int c = 0; c < MAX_PROCESSOS; c++) {
            if (cronometros_io[c] > 0) {
                cronometros_io[c]--; 
                
                if (cronometros_io[c] == 0) {
                    
                    // print com o tempo certinho
                    printf("%ds     [HW] I/O concluido no Slot %d (IRQ1)\n", mural->tempo_global, c);
                    kill(pid_so, SIGUSR2); 
                    usleep(50000); 
                }
            }
        }

        AvisoHardware pacote_recebido;
        while (msgrcv(id_caixa_msg, &pacote_recebido, sizeof(AvisoHardware) - sizeof(long), TYPE_START_IO, IPC_NOWAIT) != -1) {
            
            // mostra que recebeu com o timestamp
            printf("%ds     [HW] Novo pedido de I/O recebido (Slot %d)\n", mural->tempo_global, pacote_recebido.id_do_app);
            for (int k = 0; k < MAX_PROCESSOS; k++) {
                if (cronometros_io[k] == 0) {
                    cronometros_io[k] = 3; 
                    break;
                }
            }
        }

        kill(pid_so, SIGUSR1); 
    }

    return 0;
}