// fila.h
#ifndef FILA_H
#define FILA_H

#include "definicoes.h"

// estrutura da nossa fila circular pro disco
typedef struct {
    int lista_espera [ MAX_PROCESSOS ] ;
    int inicio;
    int fim;
    int tamanho_atual;
} FilaEsperaDisco;

// assinaturas das funcoes
void inicializar_fila(FilaEsperaDisco *f);
void colocar_no_disco(FilaEsperaDisco *f, int id_alvo);
int tirar_do_disco(FilaEsperaDisco *f);

#endif