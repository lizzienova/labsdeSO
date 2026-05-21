// aplicacao.c
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
#include "definicoes.h"

int main(int argc, char *argv [ ] ) {
    // checa se o kernel passou os argumentos direito
    if (argc < 3) {
        fprintf(stderr, "App -> Erro: Argumentos insuficientes.\n");
        return 1;
    }
    
    // converte os argumentos de string pra int
    int meu_id = atoi(argv [ 1 ] );
    int vai_usar_disco = atoi(argv [ 2 ] ); 
    
    // abre a memoria compartilhada q o kernel criou
    int fd_memoria = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd_memoria == -1) {
        perror("App -> Erro ao abrir memoria compartilhada");
        return 1;
    }
    
    // mapeia a memoria no espaco do app
    BlocoProcesso *mural = mmap(NULL, sizeof(BlocoProcesso) * MAX_PROCESSOS, PROT_READ | PROT_WRITE, MAP_SHARED, fd_memoria, 0);
    if (mural == MAP_FAILED) {
        perror("App -> Erro no mmap");
        return 1;
    }

    pid_t pid_do_chefe = getppid();
    printf("Processo A%d Inicializado.\n", meu_id + 1);

    // loop de execucao ate bater no limite do pc
    while (mural [ meu_id ] .contador_instrucoes < MAX_PC) {
        sleep(1); // simula o tempo processando
        
        mural [ meu_id ] .contador_instrucoes++; 
        printf("  -> Processo A%d Executando PC=%d\n", meu_id + 1, mural [ meu_id ] .contador_instrucoes);

        int disparar_syscall = 0;
        char letra_op = '0';

        // regra de negocio: pede io so se a flag tiver 1 e pc for 3
        if (vai_usar_disco == 1 && mural [ meu_id ] .contador_instrucoes == 3) { 
            disparar_syscall = 1;
            letra_op = (meu_id % 2 == 0) ? 'R' : 'W'; 
        }

        // se precisou de disco, grita pro kernel
        if (disparar_syscall) {
            printf("  !! Processo A%d Pedindo I/O (%c) no PC=%d\n", meu_id + 1, letra_op, mural [ meu_id ] .contador_instrucoes);
            
            mural [ meu_id ] .tipo_operacao = letra_op; 
            kill(pid_do_chefe, SIGURG); // manda a interrupcao de software
            
            usleep(50000); // delayzinho pra nao encavalar os sinais
        }
    }

    // avisa na memoria q terminou a execucao
    mural [ meu_id ] .estado_atual = TERMINATED;
    printf("Processo A%d Finalizado.\n", meu_id + 1);
    
    return 0;
}