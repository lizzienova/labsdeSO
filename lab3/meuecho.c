#include <stdio.h>

// Lis Almeida || 2421294
// Rafaela Bessa || 2420043

int main(int argc, char *argv[]) {
    // Começa do i = 1 porque o argv é o próprio nome do programa ("./meuecho")
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n"); // Quebra de linha no final
    return 0;
}