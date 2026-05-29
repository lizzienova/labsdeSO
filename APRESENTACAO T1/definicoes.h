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

// estrutura do pcb (bloco de controle)
typedef struct {
    pid_t pid_real;          
    int estado_atual;        
    int contador_instrucoes; 
    char tipo_operacao;      
} BlocoProcesso;


// agora ela guarda o 'tempo_global' pra que o kernel, o hw e os apps leiam o mesmo relogio.
typedef struct {
    int tempo_global;
    BlocoProcesso processos[MAX_PROCESSOS];
} MemoriaCompartilhada;

// estrutura da mensagem pro hardware
typedef struct {
    long tipo_msg; 
    int id_do_app; 
} AvisoHardware;

#endif