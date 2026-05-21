// definicoes.h
// Lis Almeida || 2421294
// Rafaela Bessa || 2420043

#ifndef DEFINICOES_H
#define DEFINICOES_H

#include <sys/types.h>

// limites do simulador
#define MAX_PROCESSOS 6   
#define MAX_PC 6          

// chaves do ipc
#define SHM_NAME "/shm_tabela_pcb"
#define MSG_KEY 0x123456  

// flag pra mensagem de io
#define TYPE_START_IO 1

// estados possiveis do processo
#define READY 0
#define RUNNING 1
#define BLOCKED 2
#define TERMINATED 3

// estrutura do pcb (bloco de controle do processo)
typedef struct {
    pid_t pid_real;          // pid real dado pelo linux
    int estado_atual;        // estado (ready, running, etc)
    int contador_instrucoes; // pc (program counter)
    char tipo_operacao;      // tipo de io (R ou W)
} BlocoProcesso;

// estrutura da mensagem enviada pro hardware
typedef struct {
    long tipo_msg; // tipo da mensagem pro msgsnd
    int id_do_app; // qual processo pediu o io
} AvisoHardware;

#endif