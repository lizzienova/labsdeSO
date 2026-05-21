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

// Variáveis Globais (Como é o Kernel, essas variáveis não são compartilhadas com as apps)
PCB *tabela;          // Ponteiro para a memória compartilhada.
int proc_atual = -1;  // Índice do processo que detém a CPU no momento.
int msgid = -1;       // ID da fila de mensagens IPC.
FilaIO fila_io = { .frente = 0, .tras = 0, .qtd = 0 };

// =========================================================================
// GERENCIAMENTO DA FILA FIFO (Para Processos em BLOCKED)
// =========================================================================
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
    return id; // Retorna sempre o processo que está esperando há mais tempo (FIFO).
}

// =========================================================================
// ALGORITMO DE ESCALONAMENTO (ROUND-ROBIN)
// =========================================================================
void scheduler() {
    int proximo = -1;
    
    // Busca Circular: Começa olhando o próximo elemento (proc_atual + 1).
    // O operador módulo (%) garante que se passar do limite, volta para o índice 0.
    for (int i = 1; i <= NUM_APPS; i++) {
        int idx = (proc_atual + i) % NUM_APPS;
        // O Kernel só entrega a CPU para quem está PRONTO ou já está RODANDO.
        // Processos BLOCKED (esperando disco) são completamente ignorados aqui.
        if (tabela[idx].state == READY || tabela[idx].state == RUNNING) {
            proximo = idx;
            break;
        }
    }

    if (proximo != -1 && proximo != proc_atual) {
        // PREEMPÇÃO (Context Switch): Arranca a CPU do processo atual.
        if (proc_atual != -1 && tabela[proc_atual].state == RUNNING) {
            tabela[proc_atual].state = READY; 
            // O sinal SIGSTOP congela o processo no Linux, preservando todos os registradores.
            kill(tabela[proc_atual].pid, SIGSTOP); 
            printf("[Kernel] Pausou A%d (PC=%d)\n", proc_atual + 1, tabela[proc_atual].PC);
        }
        
        // Atribui a CPU para o novo processo selecionado.
        proc_atual = proximo;
        tabela[proc_atual].state = RUNNING;
        printf("[Kernel] Retomou A%d (PC=%d)\n", proc_atual + 1, tabela[proc_atual].PC);
        // O sinal SIGCONT restaura a execução do processo exatamente de onde parou.
        kill(tabela[proc_atual].pid, SIGCONT); 
    }
}

// =========================================================================
// MANIPULADORES DE SINAIS (Handlers) - As interrupções do sistema
// =========================================================================

// Tratador do IRQ0 (Fim do Timeslice / Interrupção de Relógio)
void handle_irq0(int sig) {
    // Preempção involuntária: O processo não quis sair da CPU, mas o tempo acabou.
    scheduler(); 
}

// Tratador do IRQ1 (Término da Operação de E/S do Hardware)
void handle_irq1(int sig) {
    printf("[Kernel] IRQ1 recebido.\n");
    
    // Libera quem pediu I/O primeiro (Garante que não haja "Starvation").
    int id_liberado = pop_fila(); 
    
    if (id_liberado != -1) {
        // Restaura o contexto para a fila de execução da CPU.
        tabela[id_liberado].state = READY; 
        tabela[id_liberado].syscall_type = '0'; // Limpa o parâmetro da Syscall.
        printf("[Kernel] A%d pronto apos I/O.\n", id_liberado + 1);
        
        // Otimização de CPU: Se o sistema estava totalmente ocioso (todos bloqueados),
        // força um escalonamento imediato para não desperdiçar ciclos de relógio.
        if (proc_atual == -1 || tabela[proc_atual].state != RUNNING) {
            scheduler();
        }
    }
}

// Tratador da Syscall (A aplicação disparou um SIGURG)
void handle_syscall(int sig) {
    if (proc_atual != -1) {
        char tipo = tabela[proc_atual].syscall_type; 
        printf("[Kernel] Syscall(%c) de A%d. Bloqueando...\n", tipo, proc_atual + 1);
        
        // Preempção voluntária: O processo abdica da CPU para aguardar o disco.
        tabela[proc_atual].state = BLOCKED; 
        push_fila(proc_atual);              
        
        // Envia comando para a placa controladora (Hardware) via IPC.
        MensagemIPC msg_aviso;
        msg_aviso.msg_type = TYPE_START_IO;
        msg_aviso.id_processo = proc_atual;
        // msgsnd empilha a mensagem no SO de forma síncrona.
        msgsnd(msgid, &msg_aviso, sizeof(MensagemIPC) - sizeof(long), 0);

        // Remove o processo da CPU imediatamente.
        kill(tabela[proc_atual].pid, SIGSTOP);

        // Invoca o escalonador para que a CPU atenda o próximo processo da fila READY.
        scheduler();
    }
}

// =========================================================================
// INICIALIZAÇÃO DO NÚCLEO (Main)
// =========================================================================
int main() {
    // 1. IPC: Cria a Fila de Mensagens System V (Com permissão global 0666).
    msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    
    // 2. MEMÓRIA COMPARTILHADA: Cria o objeto de memória POSIX.
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    // Ajusta fisicamente o tamanho do arquivo virtual para caber o vetor de PCBs.
    ftruncate(shm_fd, sizeof(PCB) * MAX_PROCESSOS); 
    
    // 3. MMAP: Mapeia o arquivo virtual para o espaço de endereçamento deste processo.
    // MAP_SHARED indica que alterações feitas aqui refletirão nos processos filhos.
    tabela = mmap(NULL, sizeof(PCB) * MAX_PROCESSOS, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    
    for (int i = 0; i < MAX_PROCESSOS; i++) {
        tabela[i].pid = 0;
        tabela[i].state = READY;
        tabela[i].PC = 0;
        tabela[i].syscall_type = '0';
    }
    
    // 4. TABELA DE VETORES DE INTERRUPÇÃO (Mapeando os Sinais)
    // Sobrescreve o comportamento padrão do Linux para apontar para as nossas funções.
    signal(SIGUSR1, handle_irq0); 
    signal(SIGUSR2, handle_irq1); 
    signal(SIGURG, handle_syscall); 

    printf("Sistema iniciado.\n");

    // 5. CRIAÇÃO DE PROCESSOS (FORK + EXEC)
    for (int i = 0; i < NUM_APPS; i++) {
        pid_t pid = fork(); // Duplica o processo (cria um filho).
        if (pid == 0) {
            // Ambiente do Filho: Substitui o binário do Kernel pelo binário da Aplicação.
            char arg_id[10];
            sprintf(arg_id, "%d", i);
            execl("./app", "./app", arg_id, NULL); 
        } else {
            // Ambiente do Pai (Kernel): Registra o PID na tabela.
            tabela[i].pid = pid;
            // GARANTIA DE SINCRONISMO: Congela o processo filho imediatamente após a criação
            // para garantir que ele só execute quando o Escalonador decidir.
            kill(pid, SIGSTOP); 
        }
    }

    // Criação do Processo de Hardware Simulado
    pid_t inter_pid = fork();
    if (inter_pid == 0) {
        char arg_pid[10];
        sprintf(arg_pid, "%d", getppid()); // Passa o PID do Kernel como argumento pro Hardware
        execl("./intercontroller", "./intercontroller", arg_pid, NULL); 
    }

    // 6. BOOT: Inicia o escalonamento dando a CPU para o A1.
    proc_atual = 0;
    tabela[proc_atual].state = RUNNING;
    printf("[Kernel] Executando A1 (PC=0)\n");
    kill(tabela[proc_atual].pid, SIGCONT);

    // 7. LOOP DE SUPERVISÃO DO KERNEL
    int ativos = NUM_APPS;
    while (ativos > 0) {
        ativos = 0;
        for (int i = 0; i < NUM_APPS; i++) {
            if (tabela[i].state != TERMINATED) ativos++;
        }
        // Omission Yield: Se ainda há processos vivos, o Kernel dorme (economizando CPU do host)
        // e será acordado automaticamente quando receber qualquer sinal (IRQ0, IRQ1, SIGURG).
        pause(); 
    }

    // 8. TEARDOWN (Encerramento Limpo)
    printf("[Kernel] Todos finalizaram.\n");
    kill(inter_pid, SIGKILL); // Mata o simulador de hardware.
    
    // Libera recursos IPC do Linux para evitar vazamento de memória do hospedeiro.
    msgctl(msgid, IPC_RMID, NULL); 
    munmap(tabela, sizeof(PCB) * MAX_PROCESSOS);
    shm_unlink(SHM_NAME);
    close(shm_fd);

    printf("Sistema encerrado.\n");
    return 0;
}