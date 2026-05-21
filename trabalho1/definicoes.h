// definicoes.h
// Lis Almeida || 2421294
// Rafaela Bessa || 2420043

// Trabalho1_LisAlmeida_2421294_RafaelaBessa_2420043
// Relatoriot1_LisAlmeida_2421294_RafaelaBessa_2420043
#ifndef DEFINICOES_H
#define DEFINICOES_H

#include <sys/types.h>

// limites maximos da nossa simulacao
#define MAX_PROCESSOS 6   
#define MAX_PC 6          

// chaves pro ipc
#define SHM_NAME "/shm_tabela_pcb"
#define MSG_KEY 0x123456  

// tipo da msg que o hw le
#define TYPE_START_IO 1

// estados possiveis de um app
#define READY 0
#define RUNNING 1
#define BLOCKED 2
#define TERMINATED 3

// nosso pcb que vai ficar na memoria compartilhada
typedef struct {
    pid_t pid_real;
    int estado_atual;
    int contador_instrucoes;
    char tipo_operacao; 
} BlocoProcesso;

// struct da msg pra fila do system v
typedef struct {
    long tipo_msg;
    int id_do_app;
} AvisoHardware;

#endif