// Kernel_sim.c
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
#include "comum.h"

// Globais
PCB *tabela;
int processo_agr = -1;
int msgid = -1;

// Fila FIFO
FilaIO fila_io = { .frente = 0, .tras = 0, .qtd = 0 };

// Funções para FIFO
void push_fila(int id_processo) {
    if (fila_io.qtd < MAX_PROCESSOS) {
        fila_io.dados[fila_io.tras] = id_processo;
        fila_io.tras = (fila_io.tras + 1) % MAX_PROCESSOS;
        fila_io.qtd++;
    }
}

int retira_da_fila() {
    if (fila_io.qtd == 0) return -1;
    int id = fila_io.dados[fila_io.frente];
    fila_io.frente = (fila_io.frente + 1) % MAX_PROCESSOS;
    fila_io.qtd--;
    return id;
}

// Round-Robin
void scheduler() {
    int proximo = -1;
    
    // Procura processo
    for (int i = 1; i <= NUM_APPS; i++) {
        int idx = (processo_agr + i) % NUM_APPS;
        if (tabela[idx].state == READY || tabela[idx].state == RUNNING) {
            proximo = idx;
            break;
        }
    }

    // Se encontrou um processo diferente do atual p rodar
    if (proximo != -1 && proximo != processo_agr) {
        // Pausa quem estava em exec
        if (processo_agr != -1 && tabela[processo_agr].state == RUNNING) {
            tabela[processo_agr].state = READY;
            kill(tabela[processo_agr].pid, SIGSTOP);
            printf("[Kernel] Pausou A%d (PC=%d)\n", processo_agr + 1, tabela[processo_agr].PC);
        }
        
        processo_agr = proximo;
        tabela[processo_agr].state = RUNNING;
        printf("[Kernel] Retomou A%d (PC=%d)\n", processo_agr + 1, tabela[processo_agr].PC);
        kill(tabela[processo_agr].pid, SIGCONT);
    }
}

//func que garante a preempcao
void handle_irq0(int sig) {
    scheduler();
}

void handle_irq1(int sig) {
    printf("[Kernel] IRQ1 recebido.\n");
    
    int id_livre = retira_da_fila();
    
    if (id_livre != -1) {
        tabela[id_livre].state = READY;
        tabela[id_livre].syscall_type = '0'; // Limpa o parâmetro de syscall do contexto
        printf("[Kernel] A%d pronto apos I/O.\n", id_livre + 1);
        
        // Se a CPU estiver desocupada, força o escalonamento
        if (processo_agr == -1 || tabela[processo_agr].state != RUNNING) {
            scheduler();
        }
    } else {
        printf("[Kernel] Aviso: IRQ1 recebido, mas a fila FIFO estava vazia.\n");
    }
}

void handle_syscall(int sig) {
    if (processo_agr != -1) {
        char tipo = tabela[processo_agr].syscall_type; 
        printf("[Kernel] Syscall(%c) de A%d. Bloqueando...\n", tipo, processo_agr + 1);
        
        // Modifica pra bloqueado
        tabela[processo_agr].state = BLOCKED;
        
        // Coloca o ID do processo na Fila FIFO
        push_fila(processo_agr);
        
        MensagemIPC msg_aviso;
        msg_aviso.msg_type = TYPE_START_IO;
        msg_aviso.id_processo = processo_agr;
        if (msgsnd(msgid, &msg_aviso, sizeof(MensagemIPC) - sizeof(long), 0) == -1) {
            perror("[KERNEL] Erro ao enviar mensagem IPC para o Intercontroller");
        }

        kill(tabela[processo_agr].pid, SIGSTOP);

        scheduler();
    }
}

int main() {
    msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("[KERNEL] Erro ao criar a fila de mensagens IPC");
        exit(1);
    }
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("[KERNEL] Erro ao criar memoria compartilhada");
        exit(1);
    }
    ftruncate(shm_fd, sizeof(PCB) * MAX_PROCESSOS);
    tabela = mmap(NULL, sizeof(PCB) * MAX_PROCESSOS, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    for (int i = 0; i < MAX_PROCESSOS; i++) {
        tabela[i].pid = 0;
        tabela[i].state = READY;
        tabela[i].PC = 0;
        tabela[i].syscall_type = '0';
    }
    signal(SIGUSR1, handle_irq0);
    signal(SIGUSR2, handle_irq1);
    signal(SIGURG, handle_syscall);

    printf("Sistema iniciado.\n");

    for (int i = 0; i < NUM_APPS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char arg_id[10];
            sprintf(arg_id, "%d", i);
            execl("./app", "./app", arg_id, NULL);
            perror("[KERNEL] Erro no execl da app");
            exit(1);
        } else {
            tabela[i].pid = pid;
            kill(pid, SIGSTOP);
        }
    }

    pid_t inter_pid = fork();
    if (inter_pid == 0) {
        char arg_pid[10];
        // Pega o PID do processo pai
        sprintf(arg_pid, "%d", getppid()); 
        
        execl("./intercontroller", "./intercontroller", arg_pid, NULL); 
        
        perror("[KERNEL] Erro no execl do intercontroller");
        exit(1);
    }

    processo_agr = 0;
    tabela[processo_agr].state = RUNNING;
    printf("[Kernel] Executando A1 (PC=0)\n");
    kill(tabela[processo_agr].pid, SIGCONT);

    // Espera todas as aplicações terminarem
    int ativos = NUM_APPS;
    while (ativos > 0) {
        ativos = 0;
        for (int i = 0; i < NUM_APPS; i++) {
            if (tabela[i].state != TERMINATED) {
                ativos++;
            }
        }
        pause();
    }

    // Preparando o fim do nosso sistema
    printf("[Kernel] Todos finalizaram.\n");
    kill(inter_pid, SIGKILL);
    
    msgctl(msgid, IPC_RMID, NULL); 
    munmap(tabela, sizeof(PCB) * MAX_PROCESSOS);
    shm_unlink(SHM_NAME);
    close(shm_fd);

    printf("Sistema encerrado.\n");
    return 0;
}