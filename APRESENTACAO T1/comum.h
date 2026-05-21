#ifndef COMUM_H
#define COMUM_H

#include <sys/types.h> // Necessário para o tipo pid_t

// =========================================================================
// 1. DEFINIÇÕES GERAIS E MACROS DE ESTADO (O Ciclo de Vida do Processo)
// =========================================================================
#define READY 0      // Processo está na fila do Round-Robin esperando sua vez de usar a CPU.
#define RUNNING 1    // Processo detém o controle da CPU no milissegundo atual.
#define BLOCKED 2    // Processo sofreu preempção voluntária (I/O). Foi retirado da CPU.
#define TERMINATED 3 // Processo executou todas as instruções e "morreu".

#define MAX_PC 6          // Total de instruções que cada processo vai executar.
#define MAX_PROCESSOS 3   // Tamanho máximo das nossas estruturas de dados.
#define NUM_APPS 3        // Número de processos de aplicação vivos.

// =========================================================================
// 2. IDENTIFICADORES DE IPC (Inter-Process Communication)
// =========================================================================
// O Linux exige uma string (caminho virtual) para criar a Memória Compartilhada POSIX.
#define SHM_NAME "/mem_comp" 

// Chave estática arbitrária para o SO criar a fila de mensagens System V (o "rádio").
#define MSG_KEY 1234 

// Tipo de mensagem para o msgsnd/msgrcv. Exigência da struct de mensagens do System V.
#define TYPE_START_IO 1

// =========================================================================
// 3. ESTRUTURAS DE DADOS DO NÚCLEO
// =========================================================================

// PCB (Process Control Block): É a estrutura mais importante do SO.
// Representa o contexto mínimo que o Kernel precisa salvar quando tira um processo da CPU.
typedef struct {
    pid_t pid;         // O Identificador de Processo real mapeado no Kernel do Linux.
    int state;         // O estado na máquina de estados (READY, RUNNING, etc).
    int PC;            // Program Counter: a linha de código atual (estado da execução).
    char syscall_type; // Parâmetro da Syscall: guarda se o processo pediu 'R' (Read) ou 'W' (Write).
} PCB;

// Fila Circular (FIFO) para escalonamento de I/O.
// Escalonamento de bloqueados NÃO É Round-Robin. É FIFO (Primeiro a pedir, primeiro a ser atendido).
typedef struct {
    int dados[MAX_PROCESSOS]; // Vetor que guarda o índice dos processos bloqueados.
    int frente, tras, qtd;    // Ponteiros para gerenciar a fila circular.
} FilaIO;

// Estrutura obrigatória da Fila de Mensagens System V (IPC).
// Usada para o Kernel enviar pacotes de dados de forma assíncrona para o Hardware.
typedef struct {
    long msg_type;   // Obrigatório: Filtro do tipo de mensagem.
    int id_processo; // Payload (Carga útil): O Kernel avisa *qual* processo quer I/O.
} MensagemIPC;

#endif