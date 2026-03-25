#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_LINHA 1024
#define MAX_ARGS 100

// Lis Almeida || 2421294
// Rafaela Bessa || 2420043

int main() {
    char linha[MAX_LINHA];
    char *args[MAX_ARGS];

    while (1) {
        printf("minhashell$ ");
        
        if (fgets(linha, MAX_LINHA, stdin) == NULL) {
            break;
        }

        // Remove \n
        linha[strcspn(linha, "\n")] = 0;

        if (strlen(linha) == 0) continue;

        // Limpa o vetor args (ESSENCIAL)
        for (int j = 0; j < MAX_ARGS; j++) {
            args[j] = NULL;
        }

        // Quebra a linha em argumentos
        int i = 0;
        char *pedaco = strtok(linha, " ");
        while (pedaco != NULL && i < MAX_ARGS - 1) {
            args[i++] = pedaco;
            pedaco = strtok(NULL, " ");
        }
        args[i] = NULL;

        // Segurança extra
        if (args[0] == NULL) continue;

        // Comando exit
        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("Erro ao criar processo");
        } 
        else if (pid == 0) {
            // Filho executa o comando
            if (execvp(args[0], args) < 0) {
                printf("%s: comando nao encontrado\n", args[0]);
                exit(1);
            }
        } 
        else {
            // Pai espera
            waitpid(pid, NULL, 0);
        }
    }

    return 0;
}