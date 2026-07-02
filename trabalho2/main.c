#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// estrutura basica pra guardar os quadros da memoria fisica
typedef struct {
    int ocp; // 1 se tem algo, 0 se ta livre
    unsigned int id_pag;
    int r;
    int m;
    unsigned long t;
} Qdr;

// calcula a pagina dependendo do tamanho q o usuario escolheu
unsigned int get_pag(unsigned int ender, int tam_pag) 
{
    // corta os bits de offset
    if (tam_pag == 4) return ender >> 12;
    if (tam_pag == 8) return ender >> 13;
    
    return 0; // erro de offset
}


// algoritmo lru - procura quem tem o menor tick (foi usado a mais tempo)
unsigned int lru_sub(Qdr *mem, unsigned int tot) 
{
    unsigned int v = 0;
    unsigned long m_tmp = mem[0].t;

    for (int i = 1; i < tot; i++) {
        if (mem[i].t < m_tmp) {
            m_tmp = mem[i].t;
            v = i;
        }
    }
    return v;
}

// algoritmo nru - vai pelas 4 classes de prioridade
unsigned int nru_sub(Qdr *mem, unsigned int t) 
{
    int i;
    // prioridade 0: nao lido, nao modificado
    for (i = 0; i < t; i++) {
        if (mem[i].r == 0 && mem[i].m == 0) return i;
    }
    // prioridade 1: nao lido, mas modificado
    for (i = 0; i < t; i++) {
        if (mem[i].r == 0 && mem[i].m == 1) return i;
    }
    // prioridade 2: lido, mas nao modificado
    for (i = 0; i < t; i++) {
        if (mem[i].r == 1 && mem[i].m == 0) return i;
    }
    // prioridade 3: lido e modificado
    for (i = 0; i < t; i++) {
        if (mem[i].r == 1 && mem[i].m == 1) return i;
    }
    return 0; 
}

// algoritmo do relogio com o esquema de segunda chance
unsigned int rel_sub(Qdr *mem, unsigned int t, unsigned int *p) 
{
    while (1) {
        unsigned int i = *p;
        // se o r ta zero, achou a vitima
        if (mem[i].r == 0) {
            *p = (i + 1) % t;
            return i;
        }
        // da uma segunda chance e passa pro proximo
        mem[i].r = 0; 
        *p = (i + 1) % t;
    }
}


// algoritmo otimo - precisa prever o futuro lendo tudo
unsigned int otm_sub(Qdr *mem, unsigned int t, unsigned int *logs, unsigned long max_a, unsigned long atual) 
{
    unsigned int v = 0;
    unsigned long d_max = 0;
 
    int i;
    for (i = 0; i < t; i++) {
        unsigned int p = mem[i].id_pag;
        unsigned long p_uso = max_a; // supõe q nao vai mais usar
 
        unsigned long j = atual + 1;
        // procura quando essa pag vai aparecer de novo
        while(j < max_a) {
            if (logs[j] == p) { 
                p_uso = j; 
                break; 
            }
            j++;
        }
 
        // se nunca mais vai usar, ja pode tirar
        if (p_uso == max_a) return i; 
        // se vai demorar muito, guarda o index
        if (p_uso > d_max) {
            d_max = p_uso;
            v = i;
        }
    }
    return v;
}

int main(int argc, char *argv[]) 
{
    // testando se o cara passou todos os parametros no console
    if (argc != 5) {
        printf("Uso: %s <algoritmo> <arquivo.log> <tamanho_pagina> <tamanho_memoria>\n", argv[0]);
        return 1;
    }

    // guardando as vars
    char *alg = argv[1];
    char *arq_nome = argv[2];
    int tam_p = atoi(argv[3]);
    int tam_m = atoi(argv[4]);

    if (tam_p != 4 && tam_p != 8) {
        printf("Erro: Tamanho de pagina invalido.\n");
        return 1;
    }

    if (tam_m != 1 && tam_m != 2 && tam_m != 4) {
        printf("Erro: Tamanho de memoria invalido.\n");
        return 1;
    }

    // tenta abrir o arquivo
    FILE *f = fopen(arq_nome, "r");
    if (!f) {
        printf("Erro ao abrir o arquivo %s\n", arq_nome);
        return 1;
    }

    unsigned int qt_quadros = (tam_m * 1024) / tam_p;
    
    // alocando o vetor pra simular a memoria fisica
    Qdr *memoria = (Qdr *)calloc(qt_quadros, sizeof(Qdr));
    if (memoria == NULL) {
        printf("Erro de alocacao da memoria fisica.\n");
        fclose(f);
        return 1;
    }

    unsigned int *arr_log = NULL;
    unsigned long tot_acc = 0;
 
    // se for o algoritmo otimo tem q mapear tudo antes
    if (strcmp(alg, "Otimo") == 0) {
        unsigned int end_tmp; char op_tmp;
        
        // faz a conta de quantos acessos tem no total
        while (fscanf(f, "%x %c", &end_tmp, &op_tmp) == 2)
            tot_acc++;
        
        rewind(f);
        
        arr_log = (unsigned int *)malloc(tot_acc * sizeof(unsigned int));
        if (!arr_log) { 
            printf("Erro ao alocar o log.\n"); 
            return 1; 
        }
        
        // agora guarda eles de fato na lista
        for (unsigned long i = 0; i < tot_acc; i++) {
            (void)fscanf(f, "%x %c", &end_tmp, &op_tmp);
            arr_log[i] = get_pag(end_tmp, tam_p);
        }
        rewind(f); // volta pro comeco dnv pra simular
    }

    unsigned int faltas = 0;
    unsigned int sujas = 0;
    unsigned long tick_global = 0;
    unsigned int ptr = 0;
    unsigned int end;
    char op;

    // // dbg variable
    int x = 0;

    // loop principal: le arquivo de log
    while (fscanf(f, "%x %c", &end, &op) == 2) {
        unsigned int p = get_pag(end, tam_p);
        tick_global++;

        // reset basico de R a cada 2000 ciclos pro NRU funcionar
        if (tick_global % 2000 == 0) {
            for (unsigned int i = 0; i < qt_quadros; i++) {
                if (memoria[i].ocp) {
                    memoria[i].r = 0;
                }
            }
        }

        int achou = 0;
        
        // procura pra ver se ja ta carregado na ram (hit)
        for (unsigned int i = 0; i < qt_quadros; i++) {
            if (memoria[i].ocp && memoria[i].id_pag == p) {
                achou = 1;
                memoria[i].t = tick_global;
                memoria[i].r = 1;
                if (op == 'W') {
                    memoria[i].m = 1; // sujou a pagina
                }
                break;
            }
        }

        // nao achou = page fault
        if (achou == 0) {
            faltas++;
            int aloc = 0;

            // procura um quadro vazio antes de substituir alguem
            for (unsigned int i = 0; i < qt_quadros; i++) {
                if (memoria[i].ocp == 0) {
                    memoria[i].ocp = 1;
                    memoria[i].id_pag = p;
                    memoria[i].r = 1;
                    memoria[i].m = (op == 'W') ? 1 : 0;
                    memoria[i].t = tick_global;
                    aloc = 1;
                    break;
                }
            }

            // ram ta lotada, vamo ter q tirar um quadro
            if (aloc == 0) {
                unsigned int vitima = 0;
                int ok = 1;

                if (strcmp(alg, "LRU") == 0) {
                    vitima = lru_sub(memoria, qt_quadros);
                } else if (strcmp(alg, "NRU") == 0) {
                    vitima = nru_sub(memoria, qt_quadros);
                } else if (strcmp(alg, "Relogio") == 0) {
                    vitima = rel_sub(memoria, qt_quadros, &ptr);
                } else if (strcmp(alg, "Otimo") == 0) {
                    vitima = otm_sub(memoria, qt_quadros, arr_log, tot_acc, tick_global - 1);
                } else {
                    printf("Erro: Algoritmo nao encontrado.\n");
                    ok = 0;
                    break;
                }

                if (ok) {
                    x = vitima; // unused test
                    
                    // so contabiliza se ela ia ser escrita no disco 
                    if (memoria[vitima].m == 1) {
                        sujas++;
                    }
                    
                    // bota o novo quadro ali
                    memoria[vitima].id_pag = p;
                    memoria[vitima].r = 1;
                    memoria[vitima].m = (op == 'W') ? 1 : 0;
                    memoria[vitima].t = tick_global;
                }
            }
        }
    }

    fclose(f);

    // o pdf exige o output exatamente com essas strings
    printf("Executando o simulador...\n");
    printf("Arquivo de entrada: %s\n", arq_nome);
    printf("Tamanho da memoria fisica: %d MB\n", tam_m);
    printf("Tamanho das paginas: %d KB\n", tam_p);
    printf("Algoritmo de substituicao: %s\n", alg);
    printf("Numero de Faltas de Paginas: %u\n", faltas);
    printf("Numero de Paginas Escritas: %u\n", sujas);

    free(memoria);
    if (arr_log) {
        free(arr_log);
    }
    
    return 0;
}
