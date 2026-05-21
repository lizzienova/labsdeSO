// fila.c
#include "fila.h"

// zera os ponteiros da fila no inicio
void inicializar_fila(FilaEsperaDisco *f) {
    f->inicio = 0;
    f->fim = 0;
    f->tamanho_atual = 0;
}

// insere o processo no final da fila (push)
void colocar_no_disco(FilaEsperaDisco *f, int id_alvo) {
    if (f->tamanho_atual < MAX_PROCESSOS) {
        f->lista_espera [ f->fim ]  = id_alvo;
        f->fim = (f->fim + 1) % MAX_PROCESSOS;
        f->tamanho_atual++;
    }
}

// tira o cara que ta na frente da fila (pop)
int tirar_do_disco(FilaEsperaDisco *f) {
    if (f->tamanho_atual == 0) return -1;
    
    int id_recuperado = f->lista_espera [ f->inicio ] ;
    f->inicio = (f->inicio + 1) % MAX_PROCESSOS;
    f->tamanho_atual--;
    
    return id_recuperado;
}