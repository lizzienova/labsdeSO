#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

// Função que será chamada quando o sinal de erro matemático for emitido
void trata_fpe(int sinal) {
    printf("\n[ERRO] Excecao matematica (SIGFPE) capturada. Tentativa de divisao por zero em tipo inteiro.\n");
    exit(1); // Encerra para evitar loop
}

int main() {
    // Intercepta o sinal de Floating Point Exception
    signal(SIGFPE, trata_fpe);

    float f1, f2;
    printf("Digite dois numeros reais (ex: 5.0 0.0): ");
    scanf("%f %f", &f1, &f2);
    
    printf("\n--- RESULTADOS (REAIS) ---\n");
    printf("Soma: %f\n", f1 + f2);
    printf("Sub: %f\n", f1 - f2);
    printf("Mult: %f\n", f1 * f2);
    printf("Div: %f\n", f1 / f2); // se f2 for 0, imprime 'inf'

    int i1, i2;
    printf("\nDigite dois numeros inteiros (ex: 5 0): ");
    scanf("%d %d", &i1, &i2);

    printf("\n--- RESULTADOS (INTEIROS) ---\n");
    printf("Soma: %d\n", i1 + i2);
    printf("Sub: %d\n", i1 - i2);
    printf("Mult: %d\n", i1 * i2);
    printf("Div: %d\n", i1 / i2); // se i2 for 0, trava e o SO envia SIGFPE

    return 0;
}