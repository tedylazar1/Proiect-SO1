#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT 256
#define MAX_ARGS 20

// Functie pentru a taia stringul in bucati (argumente)
void parse_input(char *input, char **args, int *num_args) {
    *num_args = 0;
    // Eliminam \n de la final, daca exista
    input[strcspn(input, "\n")] = 0;

    char *token = strtok(input, " ");
    while (token != NULL && *num_args < MAX_ARGS - 1) {
        args[*num_args] = token;
        (*num_args)++;
        token = strtok(NULL, " ");
    }
    args[*num_args] = NULL; // Ultimul element trebuie sa fie NULL pt exec
}

int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    int num_args;

    printf("=== BINE AI VENIT IN CITY HUB ===\n");
    printf("Comenzi disponibile:\n");
    printf("1. start_monitor\n");
    printf("2. calculate_scores <district1> <district2> ...\n");
    printf("3. exit\n");

    while (1) {
        printf("\ncity_hub> ");

        // Citim comanda
        if (fgets(input, MAX_INPUT, stdin) == NULL) {
            break; // Opreste daca se apasa Ctrl+D
        }

        parse_input(input, args, &num_args);

        // Daca s-a dat doar un enter gol
        if (num_args == 0) continue;

        // --- COMANDA 1: EXIT ---
        if (strcmp(args[0], "exit") == 0) {
            printf("La revedere!\n");
            break;
        }

        // --- COMANDA 2: START MONITOR (Cu PIPES) ---
        else if (strcmp(args[0], "start_monitor") == 0) {
            int pipefd[2]; // Pipe: pipefd[0] pt citit, pipefd[1] pt scris

            // Cream teava
            if (pipe(pipefd) == -1) {
                perror("Eroare la creare pipe");
                continue;
            }

            pid_t pid = fork();

            if (pid == 0) {
                // AICI SUNTEM iN COPIL
                close(pipefd[0]); // Copilul nu citeste din teava, deci inchidem capatul de citire

                // Redirectionam iesirea standard (ecranul - stdout) catre capatul de scriere al tevii
                dup2(pipefd[1], STDOUT_FILENO);
                // Redirectionam si erorile (stderr) in aceeasi teava, ca sa prindem eroarea de PID
                dup2(pipefd[1], STDERR_FILENO);
                close(pipefd[1]);

                // inlocuim copilul cu programul monitor_reports
                execlp("./monitor_reports", "./monitor_reports", NULL);

                // Daca execlp da gres:
                perror("Eroare la lansarea monitorului");
                exit(1);
            }
            else if (pid > 0) {
                // AICI SUNTEM iN PARINTE (city_hub)
                close(pipefd[1]); // Parintele doar citeste, nu scrie in teava

                char buffer[128];
                int bytes_read;

                printf("\n--- MESAJE DE LA MONITOR ---\n");
                // Citim din teava tot ce "scuipa" monitorul pâna se inchide sau se termina afisarea de start
                // (Oprim citirea dupa câteva secunde, pentru ca monitorul ramâne blocat in pause())

                // Setam citirea sa fie non-blocanta (altfel city_hub s-ar bloca aici)
                int flags = fcntl(pipefd[0], F_GETFL, 0);
                fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

                sleep(1); // ii dam monitorului o secunda sa porneasca si sa scrie

                while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
                    buffer[bytes_read] = '\0';
                    printf("%s", buffer);
                }
                printf("----------------------------\n");

                close(pipefd[0]);

                // Nota: NU folosim wait() aici. Vrem ca monitorul sa continue sa ruleze in fundal!
            }
        }

        // --- COMANDA 3: CALCULATE SCORES (Cu DUP2) ---
        else if (strcmp(args[0], "calculate_scores") == 0) {
            if (num_args < 2) {
                printf("Eroare: Trebuie sa specifici cel putin un district.\n");
                continue;
            }

            printf("\nRezultate calcul:\n");

            // Trecem prin toate districtele date ca argumente (de la args[1] pâna la capat)
            for (int i = 1; i < num_args; i++) {
                pid_t pid = fork();

                if (pid == 0) {
                    // in Copil: Lansam scorer-ul
                    execlp("./scorer", "./scorer", args[i], NULL);
                    perror("Eroare la lansarea scorer");
                    exit(1);
                }
            }

            // Parintele trebuie sa astepte TOTI copiii (fiecare scorer) sa termine
            for (int i = 1; i < num_args; i++) {
                wait(NULL);
            }
            printf("Toate calculele au fost finalizate.\n");
        }

        // COMANDA NECUNOSCUTA
        else {
            printf("Comanda necunoscuta: %s\n", args[0]);
        }
    }

    return 0;
}
