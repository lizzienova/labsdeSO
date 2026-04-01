#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int main() {
    pid_t filho1, filho2;

    // Criação do Filho 1
    if ((filho1 = fork()) == 0) {
        char *args[] = {"yes", "--- Filho 1 Executando ---", NULL};
        execvp(args, args); // 'yes' é um utilitário Unix que imprime a string em loop
    }

    // Criação do Filho 2
    if ((filho2 = fork()) == 0) {
        char *args[] = {"yes", "+++ Filho 2 Executando +++", NULL};
        execvp(args, args);
    }

    printf("[Pai] Iniciando o escalonador por 15 segundos...\n");
    
    // Suspende ambos os processos imediatamente para evitar poluição no terminal
    kill(filho1, SIGSTOP);
    kill(filho2, SIGSTOP);

    // Escalonamento em turnos de 1 segundo
    for (int tempo = 0; tempo < 15; tempo++) {
        if (tempo % 2 == 0) {
            kill(filho2, SIGSTOP); // Pausa o 2
            kill(filho1, SIGCONT); // Continua o 1
        } else {
            kill(filho1, SIGSTOP); // Pausa o 1
            kill(filho2, SIGCONT); // Continua o 2
        }
        sleep(1); 
    }

    printf("\n[Pai] Tempo esgotado (15s). Finalizando os processos filhos...\n");
    
    // Termina os filhos de forma obrigatória
    kill(filho1, SIGKILL);
    kill(filho2, SIGKILL);

    // Evita processos zumbis
    waitpid(filho1, NULL, 0);
    waitpid(filho2, NULL, 0);

    printf("[Pai] Escalonamento concluido e filhos terminados.\n");
    return 0;
}