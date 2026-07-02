#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Representar um quadro na memória física
typedef struct {
    int ocupado; // 0 -> livre , 1 -> ocupado
    unsigned int id_pagina;
    int bit_R;
    int bit_M;
    unsigned long tempo_acesso;
} Quadro;

// Função pra calcular o número da página baseado no endereço e no tamanho da página
unsigned int calcular_pagina(unsigned int endereco, int tamanho_pagina_kb) {
    // Se a página tem 4KB, os 12 bits menos significativos são o offset
    // shift pra direita de 12 para pegar só o número da página
    if (tamanho_pagina_kb == 4) {
        return endereco >> 12;
    } 
    // Se a página tem 8KB, o offset vai usar 13 bits
    // O shift pra direita de 13 ignora o offset e deixa o número da página
    else if (tamanho_pagina_kb == 8) {
        return endereco >> 13;
    }
    return 0;
}

// Função para encontrar a vítima usando o LRU
unsigned int substituir_usando_lru(Quadro *memoria_fisica, unsigned int qttd_quadros) {
    unsigned int indice_vitima = 0;
    unsigned long menor_tempo = memoria_fisica[0].tempo_acesso;

    // Busca o usado há mais tempo
    for (unsigned int i = 1; i < qttd_quadros; i++) {
        if (memoria_fisica[i].tempo_acesso < menor_tempo) {
            menor_tempo = memoria_fisica[i].tempo_acesso;
            indice_vitima = i;
        }
    }
    return indice_vitima;
}

// Função para encontrar a vítima usando o algoritmo NRU
unsigned int substituir_usando_nru(Quadro *memoria_fisica, unsigned int qttd_quadros) {
    // Procura Classe 0 (R=0, M=0)
    for (unsigned int i = 0; i < qttd_quadros; i++) {
        if (memoria_fisica[i].bit_R == 0 && memoria_fisica[i].bit_M == 0) return i;
    }
    // Procura Classe 1 (R=0, M=1)
    for (unsigned int i = 0; i < qttd_quadros; i++) {
        if (memoria_fisica[i].bit_R == 0 && memoria_fisica[i].bit_M == 1) return i;
    }
    // Procura Classe 2 (R=1, M=0)
    for (unsigned int i = 0; i < qttd_quadros; i++) {
        if (memoria_fisica[i].bit_R == 1 && memoria_fisica[i].bit_M == 0) return i;
    }
    // Procura Classe 3 (R=1, M=1)
    for (unsigned int i = 0; i < qttd_quadros; i++) {
        if (memoria_fisica[i].bit_R == 1 && memoria_fisica[i].bit_M == 1) return i;
    }
    return 0; // fallback
}

// Função pra encontrar vitima usando Relógio
unsigned int substituir_usando_relogio(Quadro *memoria_fisica, unsigned int qttd_quadros, unsigned int *ponteiro_relogio) {
    while (1) {
        unsigned int i = *ponteiro_relogio;
        if (memoria_fisica[i].bit_R == 0) {
            *ponteiro_relogio = (i + 1) % qttd_quadros;
            return i;
        }
        memoria_fisica[i].bit_R = 0;                    // segunda chance
        *ponteiro_relogio = (i + 1) % qttd_quadros;
    }
}

//Função pra encontrar vitima usando Ótimo
unsigned int substituir_usando_otimo(Quadro *memoria_fisica, unsigned int qttd_quadros, unsigned int *log_paginas, unsigned long total_acessos, unsigned long pos_atual) {
    unsigned int vitima = 0;
    unsigned long mais_dist = 0;
 
    for (unsigned int i = 0; i < qttd_quadros; i++) {
        unsigned int pg = memoria_fisica[i].id_pagina;
        unsigned long prox_uso = total_acessos; // nunca mais usada
 
        for (unsigned long j = pos_atual + 1; j < total_acessos; j++) {
            if (log_paginas[j] == pg) { prox_uso = j; break; }
        }
 
        if (prox_uso == total_acessos) return i; // se nunca mais usada -> vítima perfeita
        if (prox_uso > mais_dist) {
            mais_dist = prox_uso;
            vitima = i;
        }
    }
    return vitima;
}

int main(int argc, char *argv[]) {
    // Validando o número de argumentos
    if (argc != 5) {
        printf("Uso: %s <algoritmo> <arquivo.log> <tamanho_pagina_kb> <tamanho_memoria_mb>\n", argv[0]);
        return 1;
    }

    char *algoritmo = argv[1];
    char *arquivo_log = argv[2];
    int tamanho_pagina = atoi(argv[3]);
    int tamanho_memoria = atoi(argv[4]);

    if (tamanho_pagina != 4 && tamanho_pagina != 8) {
        printf("Erro: O tamanho da pagina deve ser 4 ou 8 (KB).\n");
        return 1;
    }
    if (tamanho_memoria != 1 && tamanho_memoria != 2 && tamanho_memoria != 4) {
        printf("Erro: O tamanho da memoria deve ser 1, 2 ou 4 (MB).\n");
        return 1;
    }
    // Abre o arquivo de log
    FILE *arquivo = fopen(arquivo_log, "r");
    if (arquivo == NULL) {
        printf("Erro: Nao foi possivel abrir o arquivo '%s'.\n", arquivo_log);
        return 1;
    }
    unsigned int qttd_quadros = (tamanho_memoria * 1024) / tamanho_pagina;

    // Aloca o vetor de memória física e inicializa tudo com zero (ocupado = 0)
    Quadro *memoria_fisica = (Quadro *)calloc(qttd_quadros, sizeof(Quadro));
    if (memoria_fisica == NULL) {
        printf("Erro: Falha ao alocar memoria fisica.\n");
        fclose(arquivo);
        return 1;
    }

    unsigned int *log_paginas   = NULL;
    unsigned long total_acessos = 0;
 
    if (strcmp(algoritmo, "Otimo") == 0) {
        unsigned int end_tmp; char op_tmp;
        // 1ª passagem -> conta quantos acessos existem
        while (fscanf(arquivo, "%x %c", &end_tmp, &op_tmp) == 2)
            total_acessos++;
        rewind(arquivo);
        log_paginas = (unsigned int *)malloc(total_acessos * sizeof(unsigned int));
        if (!log_paginas) { printf("Erro: falha ao alocar log.\n"); return 1; }
        // 2ª passagem -> salva a sequência de páginas
        unsigned long i = 0;
        while (fscanf(arquivo, "%x %c", &end_tmp, &op_tmp) == 2) {
            log_paginas[i] = calcular_pagina(end_tmp, tamanho_pagina);
            i++;
    }
        rewind(arquivo); // volta ao início p/ simulação principal
    }

    unsigned int page_faults = 0;
    unsigned int paginas_sujas = 0;
    unsigned long tempo_global = 0;
    unsigned int ponteiro_relogio = 0;
    unsigned int endereco;
    char operacao;

    // leitura do arquivo .log
    while (fscanf(arquivo, "%x %c", &endereco, &operacao) == 2) {
        unsigned int pagina = calcular_pagina(endereco, tamanho_pagina);
        tempo_global++;

        // Reset periódico imitando a passagem de tempo para o NRU
        if (tempo_global % 2000 == 0) {
            for (unsigned int i = 0; i < qttd_quadros; i++) {
                if (memoria_fisica[i].ocupado) {
                    memoria_fisica[i].bit_R = 0;
                }
            }
        }

        int hit = 0;
        
        // Percorre a memoria fisica procurando a pagina
        for (unsigned int i = 0; i < qttd_quadros; i++) {
            if (memoria_fisica[i].ocupado && memoria_fisica[i].id_pagina == pagina) {
                // Hit
                hit = 1;
                memoria_fisica[i].tempo_acesso = tempo_global;
                memoria_fisica[i].bit_R = 1;
                if (operacao == 'W') {
                    memoria_fisica[i].bit_M = 1;
                }
                break;
            }
        }

        // Se nao encontrou, é um Page Fault
        if (!hit) {
            page_faults++;
            int alocado = 0;

            // Procura o primeiro quadro livre
            for (unsigned int i = 0; i < qttd_quadros; i++) {
                if (!memoria_fisica[i].ocupado) {
                    memoria_fisica[i].ocupado = 1;
                    memoria_fisica[i].id_pagina = pagina;
                    memoria_fisica[i].bit_R = 1;
                    memoria_fisica[i].bit_M = (operacao == 'W') ? 1 : 0;
                    memoria_fisica[i].tempo_acesso = tempo_global;
                    alocado = 1;
                    break;
                }
            }

            if (!alocado) {
                // Memoria cheia, precisa substituir
                unsigned int vitima = 0;
                int algoritmo_valido = 1;

                if (strcmp(algoritmo, "LRU") == 0) {
                    vitima = substituir_usando_lru(memoria_fisica, qttd_quadros);
                } else if (strcmp(algoritmo, "NRU") == 0) {
                    vitima = substituir_usando_nru(memoria_fisica, qttd_quadros);
                 } else if (strcmp(algoritmo, "Relogio") == 0) {
                    vitima = substituir_usando_relogio(memoria_fisica, qttd_quadros, &ponteiro_relogio);
                } else if (strcmp(algoritmo, "Otimo") == 0) {
                    vitima = substituir_usando_otimo(memoria_fisica, qttd_quadros, log_paginas, total_acessos, tempo_global - 1);
                } else {
                    printf("Erro: Algoritmo '%s' nao reconhecido/implementado.\n", algoritmo);
                    algoritmo_valido = 0;
                    break;
                }

                if (algoritmo_valido) {
                    // Verifica se a pagina que vai sair esta suja/foi modificada
                    if (memoria_fisica[vitima].bit_M == 1) {
                        paginas_sujas++;
                    }
                    
                    // Substitui pela nova pagina
                    memoria_fisica[vitima].id_pagina = pagina;
                    memoria_fisica[vitima].bit_R = 1;
                    memoria_fisica[vitima].bit_M = (operacao == 'W') ? 1 : 0;
                    memoria_fisica[vitima].tempo_acesso = tempo_global;
                }
            }
        }
    }

    fclose(arquivo);

    printf("Simulacao finalizada.\n");
    printf("Total de acessos (Tempo Global): %lu\n", tempo_global);
    printf("Total de Page Faults: %u\n", page_faults);
    printf("Total de Paginas Sujas escritas: %u\n", paginas_sujas);

    // liberando a memória
    free(memoria_fisica);
    return 0;
}
