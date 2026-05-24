#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_NAME 32
#define MAX_CATEGORY 16
#define MAX_DESC 128
typedef struct {
    int id;
    char inspector_name[MAX_NAME];
    float latitude;
    float longitude;
    char category[MAX_CATEGORY];
    int severity;
    time_t timestamp;
    char description[MAX_DESC];
} Report;

typedef struct {
    char user[32];
    int total_score;
} InspectorScore;

int main(int argc, char *argv[]) {
    // Asteptam exact un argument: numele districtului
    if (argc != 2) {
        fprintf(stderr, "Utilizare: %s <district>\n", argv[0]);
        return 1;
    }

    char *district = argv[1];
    char filepath[256];
    sprintf(filepath, "%s/reports.dat", district);

    // Deschidem fisierul binar
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        // Daca nu exista fisierul, printam o eroare si iesim
        printf("Districtul '%s' nu are rapoarte (sau nu exista).\n", district);
        return 1;
    }

    // Facem o lista ca sa tinem minte scorul fiecarui inspector (maxim 50 de inspectori pentru simplitate)
    InspectorScore scores[50];
    int num_inspectors = 0;

    Report r;
    // Citim bucata cu bucata (raport cu raport)
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        int found = 0;
        // Cautam daca inspectorul exista deja in lista noastra
        for (int i = 0; i < num_inspectors; i++) {
            if (strcmp(scores[i].user, r.inspector_name) == 0) {
                scores[i].total_score += r.severity; // Adunam severitatea
                found = 1;
                break;
            }
        }

        // Daca e un inspector nou, il adaugam la capatul listei
        if (!found && num_inspectors < 50) {
            strcpy(scores[num_inspectors].user, r.inspector_name);
            scores[num_inspectors].total_score = r.severity;
            num_inspectors++;
        }
    }

    close(fd);

    // Printam rezultatele finale
    printf("--- SCORURI PENTRU DISTRICTUL %s ---\n", district);
    if (num_inspectors == 0) {
        printf("Niciun inspector nu a raportat nimic aici.\n");
    } else {
        for (int i = 0; i < num_inspectors; i++) {
            printf("Inspector: %s | Scor total: %d\n", scores[i].user, scores[i].total_score);
        }
    }
    printf("\n");

    return 0;
}
