// kernel_sim.c
// Lis Almeida || 2421294
// Rafaela Bessa || 2420043

// Trabalho1_LisAlmeida_2421294_RafaelaBessa_2420043
// Relatoriot1_LisAlmeida_2421294_RafaelaBessa_2420043
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

BlocoProcesso *tabela_processos;
int rodando_agora = -1;
int caixa_correio = -1;
int total_programas = 0;

FilaEsperaDisco fila_hd; 

// a magica do round robin acontece aqui
void rodar_escalonador_rr() {
    int candidato = -1;
    
    // roda o array procurando o proximo q ta pronto ou rodando
    for (int i = 1; i <= total_programas; i++) {
        int indice_teste = (rodando_agora + i) % total_programas;
        if (tabela_processos [ indice_teste ] .estado_atual == READY || tabela_processos [ indice_teste ] .estado_atual == RUNNING) {
            candidato = indice_teste;
            break;
        }
    }

    // se achou alguem diferente do atual, faz a troca de contexto
    if (candidato != -1 && candidato != rodando_agora) {
        if (rodando_agora != -1 && tabela_processos [ rodando_agora ] .estado_atual == RUNNING) {
            tabela_processos [ rodando_agora ] .estado_atual = READY;
            kill(tabela_processos [ rodando_agora ] .pid_real, SIGSTOP);
            printf("Kernel -> Pausou A%d (PC=%d)\n", rodando_agora + 1, tabela_processos [ rodando_agora ] .contador_instrucoes);
        }
        
        rodando_agora = candidato;
        tabela_processos [ rodando_agora ] .estado_atual = RUNNING;
        printf("Kernel -> Retomou A%d (PC=%d)\n", rodando_agora + 1, tabela_processos [ rodando_agora ] .contador_instrucoes);
        kill(tabela_processos [ rodando_agora ] .pid_real, SIGCONT);
    }
}

// bateu o tempo (irq0), chama o escalonador
void sinal_clock_recebido(int sig) {
    rodar_escalonador_rr();
}

// disco terminou (irq1), acorda o processo
void sinal_disco_recebido(int sig) {
    printf("Kernel -> IRQ1 recebido.\n");
    
    int proc_liberado = tirar_do_disco(&fila_hd);
    
    if (proc_liberado != -1) {
        tabela_processos [ proc_liberado ] .estado_atual = READY;
        tabela_processos [ proc_liberado ] .tipo_operacao = '0'; // limpa o lixo da operacao
        printf("Kernel -> A%d pronto apos I/O.\n", proc_liberado + 1);
        printf("\n");

        // se a cpu tava de bobeira, ja bota pra rodar
        if (rodando_agora == -1 || tabela_processos [ rodando_agora ] .estado_atual != RUNNING) {
            rodar_escalonador_rr();
        }
    } else {
        printf("Kernel -> Aviso: IRQ1 recebido, mas a fila FIFO estava vazia.\n");
    }
}

// app pedindo io
void chamada_sistema_recebida(int sig) {
    if (rodando_agora != -1) {
        char req = tabela_processos [ rodando_agora ] .tipo_operacao; 
        printf("Kernel -> Syscall(%c) de A%d. Bloqueando...\n", req, rodando_agora + 1);
        
        tabela_processos [ rodando_agora ] .estado_atual = BLOCKED;
        
        colocar_no_disco(&fila_hd, rodando_agora);
        
        // manda msg pro hw avisando q tem trampo
        AvisoHardware alerta;
        alerta.tipo_msg = TYPE_START_IO;
        alerta.id_do_app = rodando_agora;
        if (msgsnd(caixa_correio, &alerta, sizeof(AvisoHardware) - sizeof(long), 0) == -1) {
            perror("Kernel -> Erro ao enviar mensagem IPC para o Intercontroller");
        }

        // congela o app e roda o proximo
        kill(tabela_processos [ rodando_agora ] .pid_real, SIGSTOP);
        rodar_escalonador_rr();
    }
}

int main(int argc, char *argv [ ] ) {
    // pega do terminal quantos apps vao rodar
    if (argc > 1) {
        total_programas = atoi(argv [ 1 ] );
    } else {
        total_programas = 3; // fallback padrao
    }

    if (total_programas > MAX_PROCESSOS) total_programas = MAX_PROCESSOS;

    inicializar_fila(&fila_hd);

    // inicializa a fila de comunicacao system v
    caixa_correio = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (caixa_correio == -1) {
        perror("Kernel-> Erro ao na fila de mensagens IPC");
        exit(1);
    }
    
    // inicializa memoria compartilhada posix
    int fd_mem_compartilhada = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd_mem_compartilhada == -1) {
        perror("Kernel-> Erro ao criar memoria compartilhada");
        exit(1);
    }
    ftruncate(fd_mem_compartilhada, sizeof(BlocoProcesso) * MAX_PROCESSOS);
    tabela_processos = mmap(NULL, sizeof(BlocoProcesso) * MAX_PROCESSOS, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem_compartilhada, 0);
    
    // zera o pcb de todo mundo
    for (int j = 0; j < MAX_PROCESSOS; j++) {
        tabela_processos [ j ] .pid_real = 0;
        tabela_processos [ j ] .estado_atual = READY;
        tabela_processos [ j ] .contador_instrucoes = 0;
        tabela_processos [ j ] .tipo_operacao = '0';
    }
    
    // mapeia os sinais
    signal(SIGUSR1, sinal_clock_recebido);
    signal(SIGUSR2, sinal_disco_recebido);
    signal(SIGURG, chamada_sistema_recebida);

    printf("Sistema iniciado com %d processos.\n", total_programas);

    // da os forks pras aplicacoes nascerem
    for (int w = 0; w < total_programas; w++) {
        pid_t pid_filho = fork();
        if (pid_filho == 0) {
            char param_id [ 20 ] ;
            char param_io [ 20 ]   = "0";
            
            sprintf(param_id, "%d", w);

            // logica hardcoded q o prof pediu pro caso de 6
            if (total_programas == 6 && (w == 2 || w == 5)) {
                sprintf(param_io, "1");
            }

            execl("./aplicacao", "./aplicacao", param_id, param_io, NULL);
            perror("Kernel -> Erro no execl da app");
            exit(1);
        } else {
            // salva o rg do filho e ja congela ele de cara
            tabela_processos [ w ] .pid_real = pid_filho;
            kill(pid_filho, SIGSTOP);
        }
    }

    // fork pro hardware nascer
    pid_t pid_hardware = fork();
    if (pid_hardware == 0) {
        char param_pid_pai [ 20 ] ;
        sprintf(param_pid_pai, "%d", getppid()); 
        
        execl("./intercontroller", "./intercontroller", param_pid_pai, NULL); 
        
        perror("Kernel -> Erro no execl do intercontroller");
        exit(1);
    }

    // da o pontape inicial no primeiro app
    rodando_agora = 0;
    tabela_processos [ rodando_agora ] .estado_atual = RUNNING;
    printf("Kernel -> Executando A1 (PC=0)\n");
    kill(tabela_processos [ rodando_agora ] .pid_real, SIGCONT);

    // fica travado aq ate geral morrer
    int rodando_ativos = total_programas;
    while (rodando_ativos > 0) {
        rodando_ativos = 0;
        for (int z = 0; z < total_programas; z++) {
            if (tabela_processos [ z ] .estado_atual != TERMINATED) {
                rodando_ativos++;
            }
        }
        pause();
    }

    // rotina de encerramento pra nao deixar vazamento de memoria
    printf("Kernel -> Todos conseguiram finalizar.\n");
    kill(pid_hardware, SIGKILL);
    
    msgctl(caixa_correio, IPC_RMID, NULL); 
    munmap(tabela_processos, sizeof(BlocoProcesso) * MAX_PROCESSOS);
    shm_unlink(SHM_NAME);
    close(fd_mem_compartilhada);

    printf("Sistema finalizado.\n");
    return 0;
}