#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_LINHA 1024
#define MAX_ARGS 100

int main() {
    char linha[MAX_LINHA];
    char *args[MAX_ARGS];

    while (1) {
        printf("minhashell$ "); // O prompt da sua shell
        
        // Lê a linha digitada pelo usuário
        if (fgets(linha, MAX_LINHA, stdin) == NULL) {
            break; // Sai se pressionar Ctrl+D
        }

        // Remove a quebra de linha (\n) que o fgets captura no final
        linha[strcspn(linha, "\n")] = 0;

        // Se o usuário só apertar Enter, não faz nada
        if (strlen(linha) == 0) continue;

        // "Pica" a string nos espaços em branco para separar comando e argumentos
        int i = 0;
        char *pedaco = strtok(linha, " ");
        while (pedaco != NULL) {
            args[i++] = pedaco;
            pedaco = strtok(NULL, " ");
        }
        args[i] = NULL; // O execvp exige que o último elemento do vetor seja NULL

        // Comando especial para fechar a nossa shell
        if (strcmp(args, "exit") == 0) {
            break; 
        }

        // --- A MÁGICA ACONTECE AQUI (A junção dos Labs anteriores com esse) ---
        pid_t pid = fork();

        if (pid < 0) {
            perror("Erro ao criar processo");
        } 
        else if (pid == 0) {
            // CÓDIGO DO FILHO
            // execvp procura o comando e o executa. Se falhar, retorna -1.
            if (execvp(args, args) < 0) {
                printf("%s: comando nao encontrado\n", args);
                exit(1);
            }
        } 
        else {
            // CÓDIGO DO PAI
            // A shell pai espera o comando (filho) terminar de rodar
            waitpid(pid, NULL, 0);
        }
    }

    return 0;
}