// fila.c
#include "fila.h"

// zera os contadores pra iniciar a fila
void inicializar_fila(FilaEsperaDisco *f) {
    f->inicio = 0;
    f->fim = 0;
    f->tamanho_atual = 0;
}

// coloca o processo no fim da fila (push)
void colocar_no_disco(FilaEsperaDisco *f, int id_alvo) {
    // checa se tem espaco antes de inserir
    if (f->tamanho_atual < MAX_PROCESSOS) {
        f->lista_espera[f->fim] = id_alvo;
        // o % garante que o indice volte pro 0 se passar do limite (circular)
        f->fim = (f->fim + 1) % MAX_PROCESSOS;
        f->tamanho_atual++;
    }
}

// tira o processo do inicio da fila (pop)
int tirar_do_disco(FilaEsperaDisco *f) {
    // se tiver vazia retorna erro
    if (f->tamanho_atual == 0) return -1;
    
    // pega o id de quem ta na frente
    int id_recuperado = f->lista_espera[f->inicio];
    // avanca o ponteiro de inicio
    f->inicio = (f->inicio + 1) % MAX_PROCESSOS;
    f->tamanho_atual--;
    
    return id_recuperado;
}