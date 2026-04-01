#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#define EVER ;;

// tratamento para o ctrl-c (SIGINT)
void intHandler(int sinal) {
    printf("\nVocê pressionou ctrl-c (%d)\n", sinal);
}

// tratamento para o ctrl-\ (SIGQUIT)
void quitHandler(int sinal) {
    printf("\nTerminando processo\n");
    exit(0);
}

int main(void) {
    void (*p)(int);
    
    // comentar esta linha para a 2ª parte do teste:
    p = signal(SIGINT, intHandler);
    printf("Endereco do manipulador anterior %p\n", p);
    
    // comentar esta linha para a 2ª parte do teste:
    p = signal(SIGQUIT, quitHandler);
    printf("Endereco do manipulador anterior %p\n", p);
    
    puts("Ctrl-c desabilitado. Use ctrl-\\ para terminar");
    
    // loop infinito
    for(EVER);
    
    return 0;
}