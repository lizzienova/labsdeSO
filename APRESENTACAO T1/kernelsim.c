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

// ponteiro pra memoria compartilhada (pcb)
BlocoProcesso *tabela_processos; 
// variavel pra guardar qual id ta rodando agora
int rodando_agora = -1;          
// id da fila de mensagens ipc
int caixa_correio = -1;          
// quantidade de processos passada no terminal
int total_programas = 0;         
// estrutura da fila do disco
FilaEsperaDisco fila_hd;         

// logica do escalonador round robin
void rodar_escalonador_rr() {
    int candidato = -1;
    
    // roda o array procurando o proximo disponivel (ready ou running)
    for (int i = 1; i <= total_programas; i++) {
        // o % faz voltar pro 0 se passar do limite
        int indice_teste = (rodando_agora + i) % total_programas;
        
        if (tabela_processos[indice_teste].estado_atual == READY || tabela_processos[indice_teste].estado_atual == RUNNING) {
            candidato = indice_teste;
            break; 
        }
    }

    // se achou um proximo diferente do atual, faz a troca de contexto
    if (candidato != -1 && candidato != rodando_agora) {
        
        // pausa quem tava rodando antes
        if (rodando_agora != -1 && tabela_processos[rodando_agora].estado_atual == RUNNING) {
            tabela_processos[rodando_agora].estado_atual = READY; 
            kill(tabela_processos[rodando_agora].pid_real, SIGSTOP); 
            printf("Kernel -> Pausou A%d (PC=%d)\n", rodando_agora + 1, tabela_processos[rodando_agora].contador_instrucoes);
        }
        
        // bota o novo candidato pra rodar
        rodando_agora = candidato;
        tabela_processos[rodando_agora].estado_atual = RUNNING; 
        printf("Kernel -> Retomou A%d (PC=%d)\n", rodando_agora + 1, tabela_processos[rodando_agora].contador_instrucoes);
        kill(tabela_processos[rodando_agora].pid_real, SIGCONT); 
    }
}

// trata o sinal do clock (irq0)
void sinal_clock_recebido(int sig) {
    // chama o escalonador pra passar a vez
    rodar_escalonador_rr(); 
}

// trata o sinal de fim de io (irq1)
void sinal_disco_recebido(int sig) {
    printf("Kernel -> IRQ1 recebido.\n");
    
    // tira o primeiro da fila do disco
    int proc_liberado = tirar_do_disco(&fila_hd);
    
    if (proc_liberado != -1) {
        // bota o processo de volta como ready e limpa o io
        tabela_processos[proc_liberado].estado_atual = READY; 
        tabela_processos[proc_liberado].tipo_operacao = '0';  
        printf("Kernel -> A%d pronto apos I/O.\n", proc_liberado + 1);
        printf("\n");

        // se a cpu estiver ociosa, ja forca o escalonamento na hora
        if (rodando_agora == -1 || tabela_processos[rodando_agora].estado_atual != RUNNING) {
            rodar_escalonador_rr();
        }
    } else {
        printf("Kernel -> Aviso: IRQ1 recebido, mas a fila FIFO estava vazia.\n");
    }
}

// trata a chamada de sistema (syscall) de io
void chamada_sistema_recebida(int sig) {
    if (rodando_agora != -1) {
        char req = tabela_processos[rodando_agora].tipo_operacao; 
        printf("Kernel -> Syscall(%c) de A%d. Bloqueando...\n", req, rodando_agora + 1);
        
        // muda o estado pra bloqueado
        tabela_processos[rodando_agora].estado_atual = BLOCKED;
        
        // poe na fila do disco
        colocar_no_disco(&fila_hd, rodando_agora);
        
        // manda mensagem pro intercontroller avisando que tem io
        AvisoHardware alerta;
        alerta.tipo_msg = TYPE_START_IO;
        alerta.id_do_app = rodando_agora;
        if (msgsnd(caixa_correio, &alerta, sizeof(AvisoHardware) - sizeof(long), 0) == -1) {
            perror("Kernel -> Erro ao enviar mensagem IPC");
        }

        // trava o aplicativo pra ele nao rodar mais
        kill(tabela_processos[rodando_agora].pid_real, SIGSTOP);
        
        // ja chama o escalonador proximo pra nao desperdicar cpu
        rodar_escalonador_rr();
    }
}

int main(int argc, char *argv[]) {
    // le a quantidade de processos passada no comando
    if (argc > 1) {
        total_programas = atoi(argv[1]);
    } else {
        total_programas = 3; // 3 por padrao
    }

    if (total_programas > MAX_PROCESSOS) total_programas = MAX_PROCESSOS;

    inicializar_fila(&fila_hd);

    // cria a fila de mensagens ipc
    caixa_correio = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (caixa_correio == -1) {
        perror("Kernel-> Erro ao na fila de mensagens IPC");
        exit(1);
    }
    
    // cria a memoria compartilhada
    int fd_mem_compartilhada = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd_mem_compartilhada == -1) {
        perror("Kernel-> Erro ao criar memoria compartilhada");
        exit(1);
    }
    
    // ajusta o tamanho da memoria pro tamanho do array de pcbs
    ftruncate(fd_mem_compartilhada, sizeof(BlocoProcesso) * MAX_PROCESSOS);
    
    // mapeia pra variavel local do kernel
    tabela_processos = mmap(NULL, sizeof(BlocoProcesso) * MAX_PROCESSOS, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem_compartilhada, 0);
    
    // inicializa o pcb de todo mundo como ready
    for (int j = 0; j < MAX_PROCESSOS; j++) {
        tabela_processos[j].pid_real = 0;
        tabela_processos[j].estado_atual = READY;
        tabela_processos[j].contador_instrucoes = 0;
        tabela_processos[j].tipo_operacao = '0';
    }
    
    // associa os sinais com as funcoes handlers
    signal(SIGUSR1, sinal_clock_recebido);
    signal(SIGUSR2, sinal_disco_recebido);
    signal(SIGURG, chamada_sistema_recebida);

    printf("Sistema iniciado com %d processos.\n", total_programas);

    // cria todos os processos de aplicacao com fork
    for (int w = 0; w < total_programas; w++) {
        pid_t pid_filho = fork(); 
        
        if (pid_filho == 0) { 
            char param_id[20];
            char param_io[20] = "0";
            
            sprintf(param_id, "%d", w);

            // se for 6 processos, passa permissao 1 pro a3 e a6
            if (total_programas == 6 && (w == 2 || w == 5)) {
                sprintf(param_io, "1");
            }

            // substitui o codigo pelo executavel da aplicacao
            execl("./aplicacao", "./aplicacao", param_id, param_io, NULL);
            perror("Kernel -> Erro no execl da app");
            exit(1);
        } else { 
            // o kernel pai salva o pid e pausa o filho
            tabela_processos[w].pid_real = pid_filho;
            kill(pid_filho, SIGSTOP);
        }
    }

    // cria o processo simulador de hardware com fork
    pid_t pid_hardware = fork(); 
    if (pid_hardware == 0) {
        char param_pid_pai[20];
        // passa o pid do kernel pra ele poder mandar sinais de volta
        sprintf(param_pid_pai, "%d", getppid()); 
        
        execl("./intercontroller", "./intercontroller", param_pid_pai, NULL); 
        
        perror("Kernel -> Erro no execl do intercontroller");
        exit(1);
    }

    // comeca rodando o primeiro da fila
    rodando_agora = 0; 
    tabela_processos[rodando_agora].estado_atual = RUNNING;
    printf("Kernel -> Executando A1 (PC=0)\n");
    kill(tabela_processos[rodando_agora].pid_real, SIGCONT); 

    // laco pra manter o kernel ativo ate os apps terminarem
    int rodando_ativos = total_programas;
    while (rodando_ativos > 0) {
        rodando_ativos = 0; 
        for (int z = 0; z < total_programas; z++) {
            if (tabela_processos[z].estado_atual != TERMINATED) {
                rodando_ativos++; 
            }
        }
        // dorme ate receber algum sinal pra nao gastar cpu
        pause(); 
    }

    // rotina de encerramento do sistema
    printf("Kernel -> Todos conseguiram finalizar.\n");
    
    // mata o hardware infinito
    kill(pid_hardware, SIGKILL);
    
    // apaga as estruturas de comunicacao
    msgctl(caixa_correio, IPC_RMID, NULL); 
    munmap(tabela_processos, sizeof(BlocoProcesso) * MAX_PROCESSOS);
    shm_unlink(SHM_NAME);
    close(fd_mem_compartilhada);

    printf("Sistema finalizado.\n");
    return 0;
}