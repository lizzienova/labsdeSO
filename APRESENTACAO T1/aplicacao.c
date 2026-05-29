// aplicacao.c
// Lis Almeida || 2421294
// Rafaela Bessa || 2420043
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "definicoes.h"

volatile sig_atomic_t recebi_tick = 0;

void tratador_tick(int sig) {
    recebi_tick = 1;
}

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;
    
    int meu_id = atoi(argv[1]);
    int vai_usar_disco = atoi(argv[2]); 
    
    int fd_memoria = shm_open(SHM_NAME, O_RDWR, 0666);
    
    
    // mapeamos a nova estrutura de memoria, que tem o relogio global junto
    MemoriaCompartilhada *mural = mmap(NULL, sizeof(MemoriaCompartilhada), PROT_READ | PROT_WRITE, MAP_SHARED, fd_memoria, 0);

    pid_t pid_do_chefe = getppid();

    signal(SIGUSR1, tratador_tick);

    
    // prints usando o relogio 
    printf("%ds     Kernel -> App A%d inicializado (PID: %d) e aguardando...\n", mural->tempo_global, meu_id + 1, getpid());

    while (mural->processos[meu_id].contador_instrucoes < MAX_PC) {
        
        while (!recebi_tick) {
            sleep(1); 
        }
        recebi_tick = 0;
        
        mural->processos[meu_id].contador_instrucoes++; 
        
        
        // print pc
        printf("%ds     App A%d -> Executando instrucao... (PC=%d)\n", 
               mural->tempo_global, meu_id + 1, mural->processos[meu_id].contador_instrucoes);

        int disparar_syscall = 0;
        char letra_op = '0';

        if (vai_usar_disco == 1 && mural->processos[meu_id].contador_instrucoes == 3) { 
            disparar_syscall = 1;
            letra_op = (meu_id % 2 == 0) ? 'R' : 'W'; 
        }

        if (disparar_syscall) {
            
            // avisa do pedido de io com o tempo certo
            printf("\n%ds     App A%d -> Chamada de Sistema (%c). Solicitando I/O...\n", 
                   mural->tempo_global, meu_id + 1, letra_op);
            
            mural->processos[meu_id].tipo_operacao = letra_op; 
            kill(pid_do_chefe, SIGURG); 
            
            
            // isso impede que o laco rode de novo e o pc aumente antes do kernel conseguir pausar de verdade.
            usleep(100000); 
        }
    }

    mural->processos[meu_id].estado_atual = TERMINATED;
    printf("%ds     App A%d -> Finalizado com sucesso.\n", mural->tempo_global, meu_id + 1);
    
    return 0;
}