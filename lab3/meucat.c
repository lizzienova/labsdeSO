#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // Lê cada arquivo passado como parâmetro
    for (int i = 1; i < argc; i++) {
        FILE *arquivo = fopen(argv[i], "r"); // "r" de read (leitura)
        
        if (arquivo == NULL) {
            printf("meucat: %s: Arquivo nao encontrado\n", argv[i]);
            continue; // Pula para o próximo arquivo, se houver
        }
        
        int caractere;
        // Lê caractere por caractere até o final do arquivo (EOF - End Of File)
        while ((caractere = fgetc(arquivo)) != EOF) {
            putchar(caractere); // Imprime na tela
        }
        
        fclose(arquivo);
    }
    return 0;
}