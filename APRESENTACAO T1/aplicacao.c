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

int main(int argc, char *argv[]) {
    // verifica se os argumentos do exec foram passados
    if (argc < 3) {
        fprintf(stderr, "App -> Erro: Argumentos insuficientes.\n");
        return 1;
    }
    
    // pega o id do processo e a permissao de io
    int meu_id = atoi(argv[1]);
    int vai_usar_disco = atoi(argv[2]); // 1 se for fazer io, 0 se nao
    
    // abre a memoria compartilhada criada pelo kernel
    int fd_memoria = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd_memoria == -1) {
        perror("App -> Erro ao abrir memoria compartilhada");
        return 1;
    }
    
    // mapeia a memoria pro ponteiro local
    BlocoProcesso *mural = mmap(NULL, sizeof(BlocoProcesso) * MAX_PROCESSOS, PROT_READ | PROT_WRITE, MAP_SHARED, fd_memoria, 0);
    if (mural == MAP_FAILED) {
        perror("App -> Erro no mmap");
        return 1;
    }

    // pega o pid do kernel pra mandar os sinais depois
    pid_t pid_do_chefe = getppid();
    printf("Processo A%d Inicializado.\n", meu_id + 1);

    // executa ate o pc chegar no maximo definido
    while (mural[meu_id].contador_instrucoes < MAX_PC) {
        // simula o tempo de execucao na cpu
        sleep(1); 
        
        // atualiza o pc na memoria compartilhada
        mural[meu_id].contador_instrucoes++; 
        printf("  -> Processo A%d Executando PC=%d\n", meu_id + 1, mural[meu_id].contador_instrucoes);

        int disparar_syscall = 0;
        char letra_op = '0';

        // checa a condicao pra pedir io (flag ativa e pc = 3)
        if (vai_usar_disco == 1 && mural[meu_id].contador_instrucoes == 3) { 
            disparar_syscall = 1;
            // define R para id par e W para id impar
            letra_op = (meu_id % 2 == 0) ? 'R' : 'W'; 
        }

        // se precisou de io, chama o kernel
        if (disparar_syscall) {
            printf("  !! Processo A%d Pedindo I/O (%c) no PC=%d\n", meu_id + 1, letra_op, mural[meu_id].contador_instrucoes);
            
            // salva a operacao no pcb
            mural[meu_id].tipo_operacao = letra_op; 
            
            // dispara o sinal urg pro kernel interceptar
            kill(pid_do_chefe, SIGURG); 
            
            // pausa rapida pra nao dar conflito de sinais no linux
            usleep(50000); 
        }
    }

    // finaliza e avisa no pcb
    mural[meu_id].estado_atual = TERMINATED;
    printf("Processo A%d Finalizado.\n", meu_id + 1);
    
    return 0;
}