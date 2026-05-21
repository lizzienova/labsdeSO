// Lis Almeida || 2421294
// Rafaela Bessa || 2420043

// Trabalho1_LisAlmeida_2421294_RafaelaBessa_2420043
// Relatoriot1_LisAlmeida_2421294_RafaelaBessa_2420043
#ifndef COMUM_H
#define COMUM_H

#include <sys/types.h>

#define MAX_PROCESSOS 6   
#define MAX_PC 6          

#define SHM_NAME "/shm_tabela_pcb"
#define MSG_KEY 0x123456  

#define TYPE_START_IO 1

#define READY 0
#define RUNNING 1
#define BLOCKED 2
#define TERMINATED 3

typedef struct {
    pid_t pid;
    int state;
    int PC;
    char syscall_type; 
} PCB;

typedef struct {
    long msg_type;
    int id_processo;
} MensagemIPC;

typedef struct {
    int dados[MAX_PROCESSOS];
    int frente;
    int tras;
    int qtd;
} FilaIO;

#endif