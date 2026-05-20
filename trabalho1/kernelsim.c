// kernel_sim.c
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

// Ponteiros e IDs globais
PCB *tabela;
int proc_atual = -1;
int msgid = -1;

// Fila FIFO interna do Kernel para gerenciar quem está aguardando I/O [cite: 32, 36]
FilaIO fila_io = { .frente = 0, .tras = 0, .qtd = 0 };

// Funções de manipulação da Fila FIFO
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

// Algoritmo de Escalonamento Round-Robin [cite: 30]
void scheduler() {
    int proximo = -1;
    
    // Procura circularmente o próximo processo pronto (READY ou RUNNING) [cite: 30]
    for (int i = 1; i <= NUM_APPS; i++) {
        int idx = (proc_atual + i) % NUM_APPS;
        if (tabela[idx].state == READY || tabela[idx].state == RUNNING) {
            proximo = idx;
            break;
        }
    }

    // Se encontrou um processo diferente do atual para rodar
    if (proximo != -1 && proximo != proc_atual) {
        // Pausa o processo que estava em execução [cite: 30]
        if (proc_atual != -1 && tabela[proc_atual].state == RUNNING) {
            tabela[proc_atual].state = READY;
            kill(tabela[proc_atual].pid, SIGSTOP); // Salva o contexto pausando [cite: 27, 30]
            printf("[KERNEL] Pausou A%d (Contexto Salvo: PC=%d)\n", proc_atual + 1, tabela[proc_atual].PC);
        }
        
        // Ativa o próximo processo [cite: 30]
        proc_atual = proximo;
        tabela[proc_atual].state = RUNNING;
        printf("[KERNEL] Escalonou A%d (Contexto Restaurado: PC=%d)\n", proc_atual + 1, tabela[proc_atual].PC);
        kill(tabela[proc_atual].pid, SIGCONT); // Restaura o contexto retomando [cite: 27, 30]
    }
}

// Tratador do IRQ0 -> Fim do Timeslice (Sinal SIGUSR1 enviado pelo Intercontroller) 
void handle_irq0(int sig) {
    printf("[KERNEL] IRQ0 Recebido (Relogio): Fim do Timeslice do processo atual.\n");
    scheduler();
}

// Tratador do IRQ1 -> Fim de I/O no Dispositivo D1 (Sinal SIGUSR2 enviado pelo Intercontroller) 
void handle_irq1(int sig) {
    printf("[KERNEL] IRQ1 Recebido: Dispositivo D1 concluiu uma operacao.\n");
    
    // Desbloqueia estritamente quem chegou primeiro na fila (Garantia FIFO) [cite: 31, 32, 37]
    int id_liberado = pop_fila();
    
    if (id_liberado != -1) {
        tabela[id_liberado].state = READY;
        tabela[id_liberado].syscall_type = '0'; // Limpa o parâmetro de syscall do contexto [cite: 50]
        printf("[KERNEL] A%d removido da fila de wait -> Estado alterado para READY.\n", id_liberado + 1);
        
        // Se a CPU estiver desocupada, força o escalonamento imediato
        if (proc_atual == -1 || tabela[proc_atual].state != RUNNING) {
            scheduler();
        }
    } else {
        printf("[KERNEL] Aviso: IRQ1 recebido, mas a fila FIFO de I/O estava vazia.\n");
    }
}

// Tratador de Syscall (Sinal SIGURG enviado pelos Processos de Aplicação) 
void handle_syscall(int sig) {
    if (proc_atual != -1) {
        char tipo = tabela[proc_atual].syscall_type; // Captura o parâmetro salvo no contexto [cite: 50]
        printf("[KERNEL] Syscall detectada de A%d (Tipo: %c). Bloqueando processo...\n", proc_atual + 1, tipo);
        
        // 1. Modifica o estado do processo para Bloqueado 
        tabela[proc_atual].state = BLOCKED;
        
        // 2. Coloca o ID do processo na Fila FIFO de Wait do Kernel [cite: 36]
        push_fila(proc_atual);
        
        // 3. Envia mensagem IPC para o Intercontroller iniciar o temporizador de hardware de 3s
        MensagemIPC msg_aviso;
        msg_aviso.msg_type = TYPE_START_IO;
        msg_aviso.id_processo = proc_atual;
        if (msgsnd(msgid, &msg_aviso, sizeof(MensagemIPC) - sizeof(long), 0) == -1) {
            perror("[KERNEL] Erro ao enviar mensagem IPC para o Intercontroller");
        }
        
        // 4. Interrompe o processo de aplicação imediatamente 
        kill(tabela[proc_atual].pid, SIGSTOP);
        
        // 5. Aciona o scheduler para passar a CPU para outro processo pronto [cite: 30]
        scheduler();
    }
}

int main() {
    // 1. Cria a Fila de Mensagens IPC (Conforme a ideia do seu amigo)
    msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("[KERNEL] Erro ao criar a fila de mensagens IPC");
        exit(1);
    }

    // 2. Cria o segmento de memória compartilhada nomeada para a Tabela de Processos
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("[KERNEL] Erro ao criar memoria compartilhada");
        exit(1);
    }
    ftruncate(shm_fd, sizeof(PCB) * MAX_PROCESSOS);
    tabela = mmap(NULL, sizeof(PCB) * MAX_PROCESSOS, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Inicializa os PCBs na tabela [cite: 44]
    for (int i = 0; i < MAX_PROCESSOS; i++) {
        tabela[i].pid = 0;
        tabela[i].state = READY;
        tabela[i].PC = 0;
        tabela[i].syscall_type = '0';
    }

    // 3. Associa as interrupções/chamadas aos tratadores de sinal corretos
    signal(SIGUSR1, handle_irq0);    // Intercontroller envia SIGUSR1 para o IRQ0 (Timeslice)
    signal(SIGUSR2, handle_irq1);    // Intercontroller envia SIGUSR2 para o IRQ1 (Fim de I/O)
    signal(SIGURG, handle_syscall);  // Aplicações enviam SIGURG para solicitar Syscall

    printf("[KERNEL] Simulador de Nucleo Operacional Iniciado com Fila FIFO.\n");

    // 4. Cria e carrega os processos de Aplicação de forma independente (fork + exec) 
    for (int i = 0; i < NUM_APPS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char arg_id[10];
            sprintf(arg_id, "%d", i);
            execl("./aplicacao", "./aplicacao", arg_id, NULL); // Transforma no binário independente 
            perror("[KERNEL] Erro no execl da aplicacao");
            exit(1);
        } else {
            tabela[i].pid = pid;
            kill(pid, SIGSTOP); // Nasce pausado aguardando escalonamento [cite: 27]
        }
    }

    // 5. Cria e carrega o Controlador de Interrupções de forma independente 
    pid_t inter_pid = fork();
    if (inter_pid == 0) {
        execl("./intercontroller", "./intercontroller", NULL); // Transforma no hardware simulado 
        perror("[KERNEL] Erro no execl do intercontroller");
        exit(1);
    }

    // 6. Dá o disparo inicial executando o processo A1
    printf("[KERNEL] Inicializando chaveamento preemptivo.\n");
    proc_atual = 0;
    tabela[proc_atual].state = RUNNING;
    printf("[KERNEL] Executando primeiro processo: A1 (PC=0)\n");
    kill(tabela[proc_atual].pid, SIGCONT); // Inicia de fato o A1 [cite: 27]

    // 7. Loop de guarda: Espera todas as aplicações terminarem [cite: 25]
    int ativos = NUM_APPS;
    while (ativos > 0) {
        ativos = 0;
        for (int i = 0; i < NUM_APPS; i++) {
            if (tabela[i].state != TERMINATED) {
                ativos++;
            }
        }
        pause(); // Dorme até a próxima interrupção (sinal) chegar
    }

    // 8. Limpeza e encerramento do sistema
    printf("[KERNEL] Todos os processos de aplicacao concluiraram. Desligando hardware...\n");
    kill(inter_pid, SIGKILL); // Encerra o loop infinito do intercontroller [cite: 25]
    
    // Libera recursos do Unix/Linux
    msgctl(msgid, IPC_RMID, NULL); // Remove a fila de mensagens IPC
    munmap(tabela, sizeof(PCB) * MAX_PROCESSOS);
    shm_unlink(SHM_NAME);
    close(shm_fd);

    printf("[KERNEL] Sistema encerrado com sucesso.\n");
    return 0;
}