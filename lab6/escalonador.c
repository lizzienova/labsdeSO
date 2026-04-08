#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

//Rafaela Bessa 2420043
//Lis Almeida 2421294

pid_t p1, p2, p3;

// rotina para tratar o ctrl-c (baseado no slide 15 do prof)
void encerra_escalonador(int sinal) {
    printf("\nmatando os filhos e encerrando...\n");
    kill(p1, SIGKILL);
    kill(p2, SIGKILL);
    kill(p3, SIGKILL);
    exit(0);
}

// rotina que os filhos vao rodar
void loop_filho(char *nome) {
    while (1) {
        printf("processo %s executando...\n", nome);
        sleep(1);
    }
}

int main(void) {
    struct timeval t;
    int ativo = 0; // 0=nenhum, 1=p1, 2=p2, 3=p3

    // intercepta o ctrl-c
    signal(SIGINT, encerra_escalonador);

    // cria os filhos
    if ((p1 = fork()) == 0) loop_filho("p1");
    if ((p2 = fork()) == 0) loop_filho("p2");
    if ((p3 = fork()) == 0) loop_filho("p3");

    // para todo mundo logo no inicio
    kill(p1, SIGSTOP);
    kill(p2, SIGSTOP);
    kill(p3, SIGSTOP);

    printf("escalonador iniciado. ctrl-c para sair.\n");

    // loop infinito do escalonador
    while (1) {
        gettimeofday(&t, NULL);
        int seg = t.tv_sec % 60; // pega o segundo atual (0-59)

        if (seg >= 5 && seg < 25) {
            if (ativo != 1) {
                printf("\nturno do p1 [%02ds]\n", seg);
                kill(p2, SIGSTOP);
                kill(p3, SIGSTOP);
                kill(p1, SIGCONT);
                ativo = 1;
            }
        } 
        else if (seg >= 45 && seg < 60) {
            if (ativo != 2) {
                printf("\nturno do p2 [%02ds]\n", seg);
                kill(p1, SIGSTOP);
                kill(p3, SIGSTOP);
                kill(p2, SIGCONT);
                ativo = 2;
            }
        } 
        else {
            if (ativo != 3) {
                printf("\n turno do p3 [%02ds]\n", seg);
                kill(p1, SIGSTOP);
                kill(p2, SIGSTOP);
                kill(p3, SIGCONT);
                ativo = 3;
            }
        }

        // dorme meio segundo 
        usleep(500000); 
    }

    return 0;
}