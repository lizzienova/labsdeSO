#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>

// Função que representa o trabalho do processo filho
void loop_filho(char *nome) {
    while (1) {
        // O filho fica em loop infinito; o SO o suspenderá via sinais
        printf("Processo %s executando...\n", nome);
        sleep(1);
    }
}

int main() {
    pid_t p1, p2, p3;
    struct timeval t;

    // Criando os processos filhos
    if ((p1 = fork()) == 0) loop_filho("P1");
    if ((p2 = fork()) == 0) loop_filho("P2");
    if ((p3 = fork()) == 0) loop_filho("P3");

    // Inicialmente, paramos todos para que o escalonador assuma o controle
    kill(p1, SIGSTOP);
    kill(p2, SIGSTOP);
    kill(p3, SIGSTOP);

    printf("Escalonador iniciado. Pressione Ctrl+C para encerrar.\n");

    while (1) {
        gettimeofday(&t, NULL);
        int segundo_atual = t.tv_sec % 60;

        /* Lógica de Escalonamento:
           P1: Inicia aos 5s, dura 20s (vai até 25s)
           P2: Inicia aos 45s, dura 15s (vai até 60s/0s)
           P3: Executa quando P1 e P2 estão parados
        */

        if (segundo_atual >= 5 && segundo_atual < 25) {
            // Período de P1
            kill(p1, SIGCONT);
            kill(p2, SIGSTOP);
            kill(p3, SIGSTOP);
        } 
        else if (segundo_atual >= 45 && segundo_atual < 60) {
            // Período de P2
            kill(p1, SIGSTOP);
            kill(p2, SIGCONT);
            kill(p3, SIGSTOP);
        } 
        else {
            // Período de P3 (Folgas entre P1 e P2)
            kill(p1, SIGSTOP);
            kill(p2, SIGSTOP);
            kill(p3, SIGCONT);
        }

        // Delay curto para evitar uso excessivo de CPU pelo escalonador
        usleep(500000); // 0.5 segundos
    }

    return 0;
}