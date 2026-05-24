#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

volatile sig_atomic_t keep_running = 1;
volatile sig_atomic_t report_received = 0;

void handle_signals(int signo) {
    if (signo == SIGINT) {
        keep_running = 0;
    } else if (signo == SIGUSR1) {
        report_received = 1;
    }
}

int main() {
    // verificam daca exista deja un monitor pornit citind pid-ul salvat
    int fd_check = open(".monitor_pid", O_RDONLY);
    if (fd_check != -1) {
        char pid_buf[32] = {0};
        read(fd_check, pid_buf, sizeof(pid_buf) - 1);
        close(fd_check);

        int old_pid = atoi(pid_buf);

        // kill cu 0 doar verifica daca procesul mai traieste, nu omoara nimic
        if (old_pid > 0 && kill(old_pid, 0) == 0) {
            printf("Error: Monitor is already running (PID: %d).\n", old_pid);
            return 1;
        }
    }

    struct sigaction sa;
    sa.sa_handler = handle_signals;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Eroare la configurarea sigaction");
        return 1;
    }

    int fd = open(".monitor_pid", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Eroare la crearea fisierului .monitor_pid");
        return 1;
    }

    char pid_str[32];
    sprintf(pid_str, "%d\n", getpid());
    write(fd, pid_str, strlen(pid_str));
    close(fd);

    printf("Monitorul a pornit (PID: %d). Astept notificari...\n", getpid());
    printf("Apasa Ctrl+C (SIGINT) in acest terminal pentru a-l opri.\n");

    while (keep_running) {
        if (report_received) {
            printf("[MONITOR] Notificare: A fost adaugat un raport nou in sistem!\n");
            report_received = 0;
        }
        pause();
    }

    printf("\n[MONITOR] Semnal SIGINT primit. Se sterge .monitor_pid si se inchide...\n");
    unlink(".monitor_pid");

    return 0;
}
