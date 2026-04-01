#include <stdio.h>
#include <signal.h>

void handler(int sinal) {
    printf("Isso nunca sera impresso.\n");
}

int main() {
    // tenta sobrescrever o tratamento do SIGKILL (sinal 9)
    if (signal(SIGKILL, handler) == SIG_ERR) {
        printf("Erro: O Sistema Operacional bloqueou a interceptação do SIGKILL.\n");
    }
    return 0;
}