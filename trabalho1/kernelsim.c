// kernel_sim.c
// Lis Almeida || 2421294
// Rafaela Bessa || 2420043

// trabalho1_LisAlmeida_2421294_RafaelaBessa_2420043
// relatoriot1_LisAlmeida_2421294_RafaelaBessa_2420043
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
int proc_atual = -1;
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

int pop_fila() {
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
        int idx = (proc_atual + i) % NUM_APPS;
        if (tabela[idx].state == READY || tabela[idx].state == RUNNING) {
            proximo = idx;
            break;
        }
    }

    // Se encontrou um processo diferente do atual p rodar
    if (proximo != -1 && proximo != proc_atual) {
        // Pausa quem estava em exec
        if (proc_atual != -1 && tabela[proc_atual].state == RUNNING) {
            tabela[proc_atual].state = READY;
            kill(tabela[proc_atual].pid, SIGSTOP);
            printf("[KERNEL] Pausou A%d (Contexto Salvo: PC=%d)\n", proc_atual + 1, tabela[proc_atual].PC);
        }
        
        proc_atual = proximo;
        tabela[proc_atual].state = RUNNING;
        printf("[KERNEL] Escalonou A%d (Contexto Restaurado: PC=%d)\n", proc_atual + 1, tabela[proc_atual].PC);
        kill(tabela[proc_atual].pid, SIGCONT);
    }
}

void handle_irq0(int sig) {
    printf("[KERNEL] IRQ0 Recebido (Relogio): Fim do Timeslice do processo atual.\n");
    scheduler();
}

void handle_irq1(int sig) {
    printf("[KERNEL] IRQ1 Recebido: Dispositivo D1 concluiu uma operacao.\n");
    
    int id_liberado = pop_fila();
    
    if (id_liberado != -1) {
        tabela[id_liberado].state = READY;
        tabela[id_liberado].syscall_type = '0'; // Limpa o parâmetro de syscall do contexto [cite: 50]
        printf("[KERNEL] A%d removido da fila de wait -> Estado alterado para READY.\n", id_liberado + 1);
        
        // Se a CPU estiver desocupada, força o escalonamento
        if (proc_atual == -1 || tabela[proc_atual].state != RUNNING) {
            scheduler();
        }
    } else {
        printf("[KERNEL] Aviso: IRQ1 recebido, mas a fila FIFO de I/O estava vazia.\n");
    }
}

void handle_syscall(int sig) {
    if (proc_atual != -1) {
        char tipo = tabela[proc_atual].syscall_type; 
        printf("[KERNEL] Syscall detectada de A%d (Tipo: %c). Bloqueando processo...\n", proc_atual + 1, tipo);
        
        // Modifica pra bloqueado
        tabela[proc_atual].state = BLOCKED;
        
        // Coloca o ID do processo na Fila FIFO
        push_fila(proc_atual);
        
        MensagemIPC msg_aviso;
        msg_aviso.msg_type = TYPE_START_IO;
        msg_aviso.id_processo = proc_atual;
        if (msgsnd(msgid, &msg_aviso, sizeof(MensagemIPC) - sizeof(long), 0) == -1) {
            perror("[KERNEL] Erro ao enviar mensagem IPC para o Intercontroller");
        }

        kill(tabela[proc_atual].pid, SIGSTOP);

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

    printf("[KERNEL] Simulador de Nucleo Operacional Iniciado com Fila FIFO.\n");

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
        // getppid() pega o PID do processo pai
        sprintf(arg_pid, "%d", getppid()); 
        
        execl("./intercontroller", "./intercontroller", arg_pid, NULL); 
        
        perror("[KERNEL] Erro no execl do intercontroller");
        exit(1);
    }

    printf("[KERNEL] Inicializando chaveamento preemptivo.\n");
    proc_atual = 0;
    tabela[proc_atual].state = RUNNING;
    printf("[KERNEL] Executando primeiro processo: A1 (PC=0)\n");
    kill(tabela[proc_atual].pid, SIGCONT);

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
    printf("[KERNEL] Todos os processos de app concluiraram. Desligando hardware...\n");
    kill(inter_pid, SIGKILL);
    
    msgctl(msgid, IPC_RMID, NULL); 
    munmap(tabela, sizeof(PCB) * MAX_PROCESSOS);
    shm_unlink(SHM_NAME);
    close(shm_fd);

    printf("[KERNEL] Sistema encerrado com sucesso.\n");
    return 0;
}