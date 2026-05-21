// fila.h
#ifndef FILA_H
#define FILA_H

#include "definicoes.h"

// estrutura da fila circular
typedef struct {
    int lista_espera[MAX_PROCESSOS]; // array pra guardar os pids
    int inicio;                      // indice do primeiro da fila
    int fim;                         // indice do proximo espaco vazio
    int tamanho_atual;               // quantidade de processos na fila
} FilaEsperaDisco;

// declaracao das funcoes
void inicializar_fila(FilaEsperaDisco *f);
void colocar_no_disco(FilaEsperaDisco *f, int id_alvo);
int tirar_do_disco(FilaEsperaDisco *f);

#endif