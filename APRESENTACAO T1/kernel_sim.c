// kernel_sim.c
// Lis Almeida || 2421294
// Rafaela Bessa || 2420043
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include "definicoes.h"
#include "fila.h"


// ponteiro agora aponta pra memoria grande que tem o tempo junto
MemoriaCompartilhada *mural; 
int rodando_agora = -1;          
int caixa_correio = -1;          
int total_programas = 0;         
FilaEsperaDisco fila_hd;         

int escalonamento_ativo = 0;

void rodar_escalonador_rr() {
    int candidato = -1;
    
    
    // acessamos o array de processos atravez do ponteiro mural
    for (int i = 1; i <= total_programas; i++) {
        int indice_teste = (rodando_agora + i) % total_programas;
        if (mural->processos[indice_teste].estado_atual == READY || mural->processos[indice_teste].estado_atual == RUNNING) {
            candidato = indice_teste;
            break; 
        }
    }

    if (candidato != -1 && candidato != rodando_agora) {
        if (rodando_agora != -1 && mural->processos[rodando_agora].estado_atual == RUNNING) {
            mural->processos[rodando_agora].estado_atual = READY; 
            
            // prints com o tempo global no inicio
            printf("\n%ds     Kernel -> Processo A%d preemptado. (PC=%d)\n", 
                   mural->tempo_global, rodando_agora + 1, mural->processos[rodando_agora].contador_instrucoes);
            kill(mural->processos[rodando_agora].pid_real, SIGSTOP); 
        }
        
        rodando_agora = candidato;
        mural->processos[rodando_agora].estado_atual = RUNNING; 
        
        // monstra quem assume a cpu de forma clara
        printf("%ds     Kernel -> Despachando Processo A%d (Retomando do PC=%d)\n", 
               mural->tempo_global, rodando_agora + 1, mural->processos[rodando_agora].contador_instrucoes);
        kill(mural->processos[rodando_agora].pid_real, SIGCONT); 
    }
}

void sinal_clock_recebido(int sig) {
    if (!escalonamento_ativo) return; 
    
    if (rodando_agora != -1 && mural->processos[rodando_agora].estado_atual == RUNNING) {
        kill(mural->processos[rodando_agora].pid_real, SIGUSR1);
        usleep(50000); 
    }
    rodar_escalonador_rr(); 
}

void sinal_disco_recebido(int sig) {
    int proc_liberado = tirar_do_disco(&fila_hd);
    if (proc_liberado != -1) {
        mural->processos[proc_liberado].estado_atual = READY; 
        mural->processos[proc_liberado].tipo_operacao = '0';  
        
        
        // formatado
        printf("%ds     Kernel -> Processo A%d pronto apos I/O e inserido na fila de prontos.\n", mural->tempo_global, proc_liberado + 1);

        if (rodando_agora == -1 || mural->processos[rodando_agora].estado_atual != RUNNING) {
            rodar_escalonador_rr();
        }
    }
}

void chamada_sistema_recebida(int sig) {
    if (rodando_agora != -1) {
        
        // formatado
        printf("%ds     Kernel -> Syscall(%c) recebida. Bloqueando A%d...\n", 
               mural->tempo_global, mural->processos[rodando_agora].tipo_operacao, rodando_agora + 1);
        
        mural->processos[rodando_agora].estado_atual = BLOCKED;
        colocar_no_disco(&fila_hd, rodando_agora);
        
        AvisoHardware alerta;
        alerta.tipo_msg = TYPE_START_IO;
        alerta.id_do_app = rodando_agora;
        msgsnd(caixa_correio, &alerta, sizeof(AvisoHardware) - sizeof(long), 0);

        kill(mural->processos[rodando_agora].pid_real, SIGSTOP);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) total_programas = atoi(argv[1]);
    else total_programas = 3; 

    if (total_programas > MAX_PROCESSOS) total_programas = MAX_PROCESSOS;

    inicializar_fila(&fila_hd);
    caixa_correio = msgget(MSG_KEY, IPC_CREAT | 0666);
    
    int fd_mem_compartilhada = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    
    // cria com o espaco da nova estrutura
    ftruncate(fd_mem_compartilhada, sizeof(MemoriaCompartilhada));
    mural = mmap(NULL, sizeof(MemoriaCompartilhada), PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem_compartilhada, 0);
    
    
    // zera o tempo logo no comeco
    mural->tempo_global = 0;

    for (int j = 0; j < MAX_PROCESSOS; j++) {
        mural->processos[j].pid_real = 0;
        mural->processos[j].estado_atual = READY;
        mural->processos[j].contador_instrucoes = 0;
        mural->processos[j].tipo_operacao = '0';
    }
    
    signal(SIGUSR1, sinal_clock_recebido);
    signal(SIGUSR2, sinal_disco_recebido);
    signal(SIGURG, chamada_sistema_recebida);

    printf("Sistema iniciado com %d processos.\n", total_programas);

    for (int w = 0; w < total_programas; w++) {
        pid_t pid_filho = fork(); 
        if (pid_filho == 0) { 
            char param_id[20], param_io[20] = "0";
            sprintf(param_id, "%d", w);
            if (total_programas == 6 && (w == 2 || w == 5)) sprintf(param_io, "1");
            execl("./aplicacao", "./aplicacao", param_id, param_io, NULL);
            exit(1);
        } else { 
            mural->processos[w].pid_real = pid_filho;
            kill(pid_filho, SIGSTOP);
        }
    }

    pid_t pid_hardware = fork(); 
    if (pid_hardware == 0) {
        char param_pid_pai[20];
        sprintf(param_pid_pai, "%d", getppid()); 
        execl("./intercontroller", "./intercontroller", param_pid_pai, NULL); 
        exit(1);
    }

    
    // a1 comecava antes do hardware dar o print de iniciado
    // o sleep(2) segura o kernel pra que as mensagens de inicializacao fiquem isoladas
    sleep(2); 
    printf("\n       Inicializacao Concluida.\n\n");

    escalonamento_ativo = 1;

    rodando_agora = 0; 
    mural->processos[rodando_agora].estado_atual = RUNNING;
    
    // o primeiro print tambem segue o relogio
    printf("%ds     Kernel -> Despachando Processo A%d (Retomando do PC=%d)\n", 
           mural->tempo_global, rodando_agora + 1, mural->processos[rodando_agora].contador_instrucoes);
    kill(mural->processos[rodando_agora].pid_real, SIGCONT); 

    int rodando_ativos = total_programas;
    while (rodando_ativos > 0) {
        rodando_ativos = 0; 
        for (int z = 0; z < total_programas; z++) {
            if (mural->processos[z].estado_atual != TERMINATED) {
                rodando_ativos++; 
            }
        }
        pause(); 
    }

    printf("\n%ds     Kernel -> Todos os processos finalizaram.\n", mural->tempo_global);
    kill(pid_hardware, SIGKILL);
    msgctl(caixa_correio, IPC_RMID, NULL); 
    munmap(mural, sizeof(MemoriaCompartilhada));
    shm_unlink(SHM_NAME);
    close(fd_mem_compartilhada);

    printf("Sistema finalizado.\n");
    return 0;
}