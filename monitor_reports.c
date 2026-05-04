#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

// Folosim variabile 'volatile sig_atomic_t' - este cea mai sigură metodă
// de a schimba valori în interiorul funcțiilor de semnal (cerință de nota 10)
volatile sig_atomic_t keep_running = 1;
volatile sig_atomic_t report_received = 0;

// Aceasta este funcția "handler" care prinde semnalele
void handle_signals(int signo) {
    if (signo == SIGINT) {
        // Dacă primește SIGINT (Ctrl+C), oprim bucla [cite: 112]
        keep_running = 0;
    } else if (signo == SIGUSR1) {
        // Dacă primește SIGUSR1, înregistrăm că a venit un raport nou [cite: 113]
        report_received = 1;
    }
}

int main() {
    // 1. Configurăm prinderea semnalelor folosind sigaction() (NU signal()) [cite: 118]
    struct sigaction sa;
    sa.sa_handler = handle_signals; // Funcția care se execută când vine semnalul
    sigemptyset(&sa.sa_mask);       // Curățăm masca de semnale
    sa.sa_flags = 0;

    // Înregistrăm acțiunile pentru cele două semnale
    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Eroare la configurarea sigaction");
        return 1;
    }

    // 2. Creăm/Suprascriem fișierul ascuns .monitor_pid [cite: 109]
    // O_TRUNC asigură că dacă fișierul exista dinainte, e șters și rescris
    int fd = open(".monitor_pid", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Eroare la crearea fisierului .monitor_pid");
        return 1;
    }

    // Luăm PID-ul programului curent și îl scriem în fișier ca text
    char pid_str[32];
    sprintf(pid_str, "%d\n", getpid());
    write(fd, pid_str, strlen(pid_str));
    close(fd);

    printf("Monitorul a pornit (PID: %d). Astept notificari...\n", getpid());
    printf("Apasa Ctrl+C (SIGINT) in acest terminal pentru a-l opri.\n");

    // 3. Bucla infinită care ține programul deschis
    while (keep_running) {
        // Verificăm dacă a venit semnalul SIGUSR1
        if (report_received) {
            printf("[MONITOR] Notificare: A fost adaugat un raport nou in sistem!\n");
            report_received = 0; // Resetăm starea ca să ascultăm pentru următorul
        }

        // pause() pune programul la somn până vine un semnal.
        // Asta face ca programul să consume 0% procesor cât stă degeaba!
        pause();
    }

    // 4. Partea de final: Programul a primit SIGINT și a ieșit din buclă [cite: 112]
    printf("\n[MONITOR] Semnal SIGINT primit. Se sterge .monitor_pid si se inchide...\n");
    unlink(".monitor_pid"); // Ștergem fișierul [cite: 111]

    return 0;
}
