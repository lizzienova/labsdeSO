#define SHM_NAME "/shm_tabela_pcb"
#ifndef COMUM_H
#define COMUM_H

#include <sys/types.h>

#define NUM_APPS 3    
#define MAX_PC 6        //aqui é para definir a quantidade de processos já que tem que ser de 3 a 6
#define MAX_FILA NUM_APPS

//Estados dos processos
#define READY 0
#define RUNNING 1
#define BLOCKED 2
#define TERMINATED 3

//Bloco de Controle de Processo
typedef struct {
    pid_t pid;
    int state;
    int PC;
    char syscall_type; 
} PCB;

//Estrutura p Fila FIFO
typedef struct {
    int dados[MAX_FILA];
    int frente;
    int tras;
    int qtd;
} FilaIO;

#endif