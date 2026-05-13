// Lis Almeida || 2421294
// Rafaela Bessa || 2420043

// trabalho1_LisAlmeida_2421294_RafaelaBessa_2420043
// relatoriot1_LisAlmeida_2421294_RafaelaBessa_2420043
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mman.h>

#define NUM_APPS 3
#define MAX_PC 6

// status do processo
#define READY 0
#define RUNNING 1
#define BLOCKED 2
#define TERMINATED 3

// pcb basico
typedef struct {
    pid_t pid;
    int state;
    int PC;
    char syscall_type; 
} PCB;

PCB *tabela;
int proc_atual = -1;

// funcao de escalonamento (round-robin)
void scheduler() {
    int proximo = -1;
    
    // busca o proximo na fila q ta pronto ou rodando
    for(int i = 1; i <= NUM_APPS; i++) {
        int idx = (proc_atual + i) % NUM_APPS;
        if(tabela[idx].state == READY || tabela[idx].state == RUNNING) {
            proximo = idx;
            break;
        }
    }

    if(proximo != -1 && proximo != proc_atual) {
        // pausa quem ta rodando
        if(proc_atual != -1 && tabela[proc_atual].state == RUNNING) {
            tabela[proc_atual].state = READY;
            kill(tabela[proc_atual].pid, SIGSTOP);
            printf("Kernel: pausou A%d (PC=%d)\n", proc_atual+1, tabela[proc_atual].PC);
        }
        
        // bota o proximo pra rodar
        proc_atual = proximo;
        tabela[proc_atual].state = RUNNING;
        printf("Kernel: retomou A%d (PC=%d)\n", proc_atual+1, tabela[proc_atual].PC);
        kill(tabela[proc_atual].pid, SIGCONT);
    }
}

// timer do quantum estourou (1s)
void handle_irq0(int sig) {
    scheduler();
}

// disco avisou q terminou o i/o
void handle_irq1(int sig) {
    printf("Kernel: IRQ1 recebido (disco liberado)\n");
    
    // acorda o primeiro q tiver bloqueado
    for(int i = 0; i < NUM_APPS; i++) {
        if(tabela[i].state == BLOCKED) {
            tabela[i].state = READY;
            tabela[i].syscall_type = 'N';
            printf("Kernel: A%d saiu do bloqueio p/ ready\n", i+1);
            break; 
        }
    }
}

// app pediu i/o
void handle_syscall(int sig) {
    if(proc_atual != -1) {
        printf("Kernel: syscall de A%d. Bloqueando...\n", proc_atual+1);
        tabela[proc_atual].state = BLOCKED;
        kill(tabela[proc_atual].pid, SIGSTOP);
        scheduler(); // passa a cpu pra outro
    }
}

// hardware simulado
void intercontroller() {
    int io_timer = 0;
    while(1) {
        sleep(1); 
        kill(getppid(), SIGALRM); // timer do quantum

        // checa se tem gente esperando i/o
        int tem_bloqueado = 0;
        for(int i = 0; i < NUM_APPS; i++) {
            if(tabela[i].state == BLOCKED) tem_bloqueado = 1;
        }

        if(tem_bloqueado) {
            io_timer++;
            if(io_timer >= 3) {
                kill(getppid(), SIGUSR2); // manda irq1 dps de 3s
                io_timer = 0;
            }
        } else {
            io_timer = 0;
        }
    }
}

// loop do programa
void aplicacao(int id) {
    while(tabela[id].PC < MAX_PC) {
        tabela[id].PC++;
        printf("  App A%d: instrucao PC=%d\n", id+1, tabela[id].PC);
        sleep(1);

        // simula pedido de leitura no meio da execucao
        if (tabela[id].PC == 3) {
            tabela[id].syscall_type = 'R'; 
            kill(getppid(), SIGUSR1); 
            usleep(100000); // delay pro kernel processar o sinal
        }
    }
    tabela[id].state = TERMINATED;
    printf("  App A%d: terminou tudo\n", id+1);
    exit(0);
}

int main() {
    // aloca memoria compartilhada
    tabela = mmap(NULL, sizeof(PCB) * NUM_APPS, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    for(int i = 0; i < NUM_APPS; i++) {
        tabela[i].state = READY;
        tabela[i].PC = 0;
        tabela[i].syscall_type = 'N';
    }

    signal(SIGALRM, handle_irq0);    
    signal(SIGUSR2, handle_irq1);    
    signal(SIGUSR1, handle_syscall); 

    printf("Iniciando sistema...\n");

    // cria os processos
    for(int i = 0; i < NUM_APPS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            aplicacao(i); 
        } else {
            tabela[i].pid = pid; 
            kill(pid, SIGSTOP);  // nasce pausado
        }
    }

    // cria controlador de hardware
    pid_t inter_pid = fork();
    if (inter_pid == 0) {
        intercontroller();
    }

    // da o start
    proc_atual = 0;
    tabela->state = RUNNING;
    kill(tabela->pid, SIGCONT);
    printf("Kernel: start no A1 (PC=0)\n");

    // espera td mundo acabar
    int ativos = NUM_APPS;
    while(ativos > 0) {
        ativos = 0;
        for(int i = 0; i < NUM_APPS; i++) {
            if(tabela[i].state != TERMINATED) ativos++;
        }
        pause(); 
    }

    printf("Todos finalizaram. Desligando...\n");
    kill(inter_pid, SIGKILL); 
    munmap(tabela, sizeof(PCB) * NUM_APPS);

    return 0;
}