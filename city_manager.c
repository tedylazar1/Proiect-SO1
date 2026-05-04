#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

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
// Funcție care transformă permisiunile (st_mode) într-un string lizibil ("rw-r--r--")
void print_permissions(mode_t mode) {
    char perms[10] = "---------";

    // Verificăm permisiunile pentru User (Proprietar)
    if (mode & S_IRUSR) perms[0] = 'r';
    if (mode & S_IWUSR) perms[1] = 'w';
    if (mode & S_IXUSR) perms[2] = 'x';

    // Verificăm permisiunile pentru Group
    if (mode & S_IRGRP) perms[3] = 'r';
    if (mode & S_IWGRP) perms[4] = 'w';
    if (mode & S_IXGRP) perms[5] = 'x';

    // Verificăm permisiunile pentru Others (Restul)
    if (mode & S_IROTH) perms[6] = 'r';
    if (mode & S_IWOTH) perms[7] = 'w';
    if (mode & S_IXOTH) perms[8] = 'x';

    printf("%s", perms);
}
// Funcție generată de AI pentru a sparge string-ul "camp:operator:valoare"
int parse_condition(const char *input, char *field, char *op, char *value) {
    // Folosim sscanf pentru a extrage cuvintele separate de caracterul ':'
    // %[^:] înseamnă "citește tot până întâlnești două puncte"
    if (sscanf(input, "%[^:]:%[^:]:%s", field, op, value) == 3) {
        return 1; // Succes, am găsit toate cele 3 părți
    }
    return 0; // Eroare de parsare
}

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    // Verificăm severitatea (trebuie convertită din text în număr)
    if (strcmp(field, "severity") == 0) {
        int sev = atoi(value);
        if (strcmp(op, "==") == 0) return r->severity == sev;
        if (strcmp(op, "!=") == 0) return r->severity != sev;
        if (strcmp(op, ">=") == 0) return r->severity >= sev;
        if (strcmp(op, "<=") == 0) return r->severity <= sev;
        if (strcmp(op, ">") == 0) return r->severity > sev;
        if (strcmp(op, "<") == 0) return r->severity < sev;
    }
    // Verificăm categoria (comparație directă de text)
    else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->category, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->category, value) != 0;
    }
    // Verificăm inspectorul
    else if (strcmp(field, "inspector") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->inspector_name, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->inspector_name, value) != 0;
    }

    return 0; // Returnăm 0 dacă nu se potrivește sau câmpul e invalid
}
int main(int argc, char *argv[]) {
    int target_id = 0;
    int threshold_val = 0;
    char *role = NULL;
    char *user = NULL;
    char *command = NULL;
    char *district = NULL;
    char *condition = NULL;
    int monitor_status = -1; // -1 = inactiv, 0 = esec, 1 = succes

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0 && i + 1 < argc) {
            role = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) {
            user = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "--add") == 0 && i + 1 < argc) {
            command = "add";
            district = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "--list") == 0 && i + 1 < argc) {
            command = "list";
            district = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "--remove_report") == 0 && i + 2 < argc) {
            command = "remove_report";
            district = argv[i + 1];
            // Funcția atoi transformă textul "2" din comandă în numărul întreg 2
            target_id = atoi(argv[i + 2]);
            i += 2; // Sărim peste 2 cuvinte (districtul și id-ul)
        }
        else if (strcmp(argv[i], "--view") == 0 && i + 2 < argc) {
            command = "view";
            district = argv[i + 1];
            target_id = atoi(argv[i + 2]);
            i += 2;
        }
        else if (strcmp(argv[i], "--update_threshold") == 0 && i + 2 < argc) {
            command = "update_threshold";
            district = argv[i + 1];
            threshold_val = atoi(argv[i + 2]); // Transformăm textul în număr
            i += 2;
        }
        else if (strcmp(argv[i], "--filter") == 0 && i + 2 < argc) {
            command = "filter";
            district = argv[i + 1];
            condition = argv[i + 2]; // ex: "severity:>=:2"
            i += 2;
        }
        else if (strcmp(argv[i], "--remove_district") == 0 && i + 1 < argc) {
            command = "remove_district";
            district = argv[i + 1];
            i++;
        }
    }

    printf("--- TESTARE ARGUMENTE ---\n");
    printf("Rol primit: %s\n", role ? role : "Nespecificat");
    printf("User primit: %s\n", user ? user : "Nespecificat");
    printf("Comanda: %s\n", command ? command : "Nespecificata");
    printf("District: %s\n", district ? district : "Nespecificat");
    printf("Dimensiunea unui raport: %lu bytes\n", sizeof(Report));

    // Asigură-te că am primit un district din linia de comandă
    if (district != NULL) {
        if (command != NULL && strcmp(command, "remove_district") == 0) {
            // Verificăm dacă e manager [cite: 107]
            if (role == NULL || strcmp(role, "manager") != 0) {
                printf("Eroare de permisiune: Doar managerul poate sterge un district!\n");
                return 1;
            }

            // Creăm un proces copil [cite: 105]
            pid_t pid = fork();

            if (pid == 0) {
                // AICI SUNTEM IN COPIL
                // execlp înlocuiește copilul cu comanda "rm -rf <district>" [cite: 105]
                execlp("rm", "rm", "-rf", district, NULL);

                // Dacă execlp dă greș, printăm eroare (normal nu ajunge aici)
                perror("Eroare la comanda rm");
                exit(1);
            }
            else if (pid > 0) {
                // AICI SUNTEM IN PĂRINTE
                wait(NULL); // Așteptăm să termine copilul ștergerea

                // Acum ștergem și scurtătura symlink [cite: 104]
                char symlink_name[256];
                sprintf(symlink_name, "active_reports-%s", district);
                unlink(symlink_name);

                printf("Districtul '%s' si scurtatura au fost sterse cu succes.\n", district);
            }
            else {
                perror("Eroare la fork");
            }

            // Oprim programul aici. Nu vrem sa mearga mai jos sa faca mkdir!
            return 0;
        }
        // Încercăm să creăm directorul cu permisiunile 0750 (zero în față înseamnă sistem octal)
        int status = mkdir(district, 0750);

        if (status == 0) {
            printf("Directorul '%s' a fost creat cu succes!\n", district);
        } else {
            // Dacă funcția nu returnează 0, cel mai probabil folderul există deja
            printf("Directorul '%s' exista deja.\n", district);
        }

        // Construim căile (paths) pentru cele 3 fișiere specifice districtului
        char reports_path[256];
        char config_path[256];
        char log_path[256];

        // sprintf funcționează ca printf, dar scrie textul într-o variabilă, nu pe ecran
        sprintf(reports_path, "%s/reports.dat", district);
        sprintf(config_path, "%s/district.cfg", district);
        sprintf(log_path, "%s/logged_district", district);

        printf("Calea pentru fisierul binar este: %s\n", reports_path);
        // 1. Creăm / Deschidem fișierul reports.dat
        // O_CREAT = creează fișierul dacă nu există
        // O_RDWR = deschide pentru citire (Read) și scriere (Write)
        // O_APPEND = orice scriem se va adăuga la sfârșitul fișierului, nu va șterge ce era înainte
        int fd_reports = open(reports_path, O_CREAT | O_RDWR, 0664);

        if (fd_reports == -1) {
            perror("Eroare la deschiderea/crearea reports.dat"); // afișează eroarea exactă
            return 1;
        }

        if (chmod(reports_path, 0664) == 0) {
            printf("Permisiunile pentru %s au fost setate la 0664.\n", reports_path);
        } else {
            perror("Eroare la setarea permisiunilor");
        }

        // Verificăm dacă utilizatorul a cerut comanda "add"
        if (strcmp(command, "add") == 0) {
            Report new_report;
            // Curățăm memoria cu zero pentru a nu avea "gunoi" în fișier
            memset(&new_report, 0, sizeof(Report));

// Aflăm dimensiunea curentă a fișierului mutând cursorul la final (SEEK_END)
            off_t file_size = lseek(fd_reports, 0, SEEK_END);

// Calculăm ID-ul corect citind ultimul raport existent
            int next_id = 1; // Presupunem că e primul
            if (file_size > 0) {
                Report last_report;
                // Mutăm cursorul înapoi cu 200 bytes față de finalul fișierului
                lseek(fd_reports, -sizeof(Report), SEEK_END);
                read(fd_reports, &last_report, sizeof(Report));
                next_id = last_report.id + 1;
            }
            new_report.id = next_id;            // Numele inspectorului este preluat din parametrul --user
            if (user != NULL) {
                // Folosim strncpy pentru siguranță, ca să nu depășim MAX_NAME
                strncpy(new_report.inspector_name, user, MAX_NAME - 1);
            } else {
                strcpy(new_report.inspector_name, "Necunoscut");
            }

            new_report.latitude = 44.42;  // Coordonate dummy
            new_report.longitude = 26.10;
            strcpy(new_report.category, "road");
            new_report.severity = 2;
            new_report.timestamp = time(NULL); // Preia timpul exact din acest moment
            strcpy(new_report.description, "Groapa pe strada principala");
            lseek(fd_reports, 0, SEEK_END);
            // Scriem efectiv blocul de date în fișier (la sfârșit, datorită flag-ului O_APPEND)
            ssize_t bytes_written = write(fd_reports, &new_report, sizeof(Report));

            if (bytes_written == sizeof(Report)) {
                printf("Raportul a fost adaugat cu succes in %s!\n", reports_path);
                //  Notificarea monitorului
                monitor_status = 0; // Presupunem că eșuează inițial

                // Încercăm să deschidem fișierul ascuns din folderul rădăcină
                int fd_pid = open(".monitor_pid", O_RDONLY);
                if (fd_pid != -1) {
                    char pid_buf[32] = {0};
                    read(fd_pid, pid_buf, sizeof(pid_buf) - 1);
                    close(fd_pid);

                    int monitor_pid = atoi(pid_buf);

                    // Funcția kill(pid, semnal) trimite semnalul!
                    // Returnează 0 dacă a reușit
                    if (monitor_pid > 0 && kill(monitor_pid, SIGUSR1) == 0) {
                        monitor_status = 1; // Succes!
                        printf("Monitorul a fost notificat cu succes!\n");
                    }
                }

                if (monitor_status == 0) {
                    printf("Avertisment: Monitorul nu este pornit sau nu a putut fi notificat.\n");
                }

            } else {
                perror("Eroare la scrierea in fisier");
            }
        }
        // Verificăm dacă utilizatorul a cerut comanda "list"
        else if (strcmp(command, "list") == 0) {

            // 1. Prima dată, citim metadatele fișierului reports.dat folosind stat()
            struct stat file_stats;
            if (stat(reports_path, &file_stats) == 0) {
                printf("\n--- INFO FISIER %s ---\n", reports_path);
                printf("Permisiuni: ");
                print_permissions(file_stats.st_mode); // Folosim funcția noastră de mai sus
                printf("\n");
                printf("Dimensiune: %ld bytes\n", file_stats.st_size);

                // Convertim timpul de modificare într-un format text lizibil
                char time_str[64];
                struct tm *timeinfo = localtime(&file_stats.st_mtime);
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
                printf("Ultima modificare: %s\n\n", time_str);
            }

            // 2. Acum citim efectiv rapoartele din interior
            // Deoarece noi am deschis fișierul cu O_APPEND, cursorul intern este la FINALUL fișierului.
            // Pentru a-l putea citi de la început, trebuie să mutăm cursorul înapoi la byte-ul 0 folosind lseek()
            lseek(fd_reports, 0, SEEK_SET);

            Report current_report;
            int report_count = 0;

            printf("--- LISTA RAPOARTE ---\n");
            // Funcția read() va citi câte 200 de bytes (sau cât e sizeof) și îi va băga în variabila current_report
            // Când ajunge la finalul fișierului, read() returnează 0 și bucla se oprește.
            while (read(fd_reports, &current_report, sizeof(Report)) == sizeof(Report)) {
                report_count++;
                printf("Raport #%d | Inspector: %s | Categorie: %s | Severitate: %d | Descriere: %s\n",
                       current_report.id,
                       current_report.inspector_name,
                       current_report.category,
                       current_report.severity,
                       current_report.description);
            }

            if (report_count == 0) {
                printf("Nu exista niciun raport in acest district.\n");
            } else {
                printf("Total rapoarte: %d\n", report_count);
            }
        }
// Verificăm dacă e comanda remove_report
        else if (strcmp(command, "remove_report") == 0) {

            // 1. Verificăm rolul (Pt ca zice proiectul ca doar managerii sterg)
            if (role == NULL || strcmp(role, "manager") != 0) {
                printf("Eroare de permisiune: Doar utilizatorii cu rolul 'manager' pot sterge rapoarte!\n");
            } else {
                lseek(fd_reports, 0, SEEK_SET); // Ne mutăm la începutul fișierului
                Report temp;
                off_t found_offset = -1;

                // 2. Căutăm raportul cu ID-ul cerut
                while (read(fd_reports, &temp, sizeof(Report)) == sizeof(Report)) {
                    if (temp.id == target_id) {
                        // Am găsit raportul! Salvăm poziția de la începutul lui
                        found_offset = lseek(fd_reports, 0, SEEK_CUR) - sizeof(Report);
                        break;
                    }
                }

                if (found_offset == -1) {
                    printf("Raportul cu ID %d nu a fost gasit in districtul %s.\n", target_id, district);
                } else {
                    // 3. Mutăm (shiftăm) toate rapoartele următoare cu o poziție în spate
                    off_t read_pos = found_offset + sizeof(Report);
                    off_t write_pos = found_offset;

                    while (lseek(fd_reports, read_pos, SEEK_SET) != -1 && read(fd_reports, &temp, sizeof(Report)) == sizeof(Report)) {
                        lseek(fd_reports, write_pos, SEEK_SET);
                        write(fd_reports, &temp, sizeof(Report));

                        read_pos += sizeof(Report);
                        write_pos += sizeof(Report);
                    }

                    // 4. Tăiem ultimul bloc duplicat rămas la final
                    off_t current_size = lseek(fd_reports, 0, SEEK_END);
                    ftruncate(fd_reports, current_size - sizeof(Report));

                    printf("Raportul #%d a fost sters cu succes. Fisierul a fost scurtat.\n", target_id);
                }
            }
        }
        // Verificăm dacă e comanda view
        else if (strcmp(command, "view") == 0) {
            lseek(fd_reports, 0, SEEK_SET); // Ne mutăm la începutul fișierului
            Report temp;
            int found = 0;

            // Căutăm raportul cu ID-ul cerut
            while (read(fd_reports, &temp, sizeof(Report)) == sizeof(Report)) {
                if (temp.id == target_id) {
                    found = 1;
                    printf("--- DETALII RAPORT #%d ---\n", temp.id);
                    printf("Inspector: %s\n", temp.inspector_name);
                    printf("Categorie: %s\n", temp.category);
                    printf("Severitate: %d/3\n", temp.severity);
                    printf("Coordonate: %.4f, %.4f\n", temp.latitude, temp.longitude);

                    // Convertim timestamp-ul într-un format lizibil
                    char time_str[64];
                    struct tm *timeinfo = localtime(&temp.timestamp);
                    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
                    printf("Data raportării: %s\n", time_str);

                    printf("Descriere: %s\n", temp.description);
                    break; // Am găsit ce căutam, ieșim din buclă
                }
            }

            if (!found) {
                printf("Raportul cu ID %d nu a fost gasit in districtul %s.\n", target_id, district);
            }
        }
        // Verificăm dacă e comanda filter
        else if (strcmp(command, "filter") == 0) {
            if (condition == NULL) {
                printf("Eroare: Trebuie sa oferi o conditie pentru filtrare!\n");
            } else {
                char field[32], op[4], value[64];

                // 1. Parsăm condiția de la tastatură
                if (parse_condition(condition, field, op, value) == 1) {
                    printf("Filtram dupa: Camp=%s | Operator=%s | Valoare=%s\n", field, op, value);
                    printf("--- REZULTATE FILTRARE ---\n");

                    lseek(fd_reports, 0, SEEK_SET); // Ne mutăm la început
                    Report temp;
                    int match_count = 0;

                    // 2. Citim rapoartele rând pe rând
                    while (read(fd_reports, &temp, sizeof(Report)) == sizeof(Report)) {
                        // 3. Testăm raportul cu funcția AI-ului
                        if (match_condition(&temp, field, op, value) == 1) {
                            match_count++;
                            printf("-> [Raport #%d] %s (%s) - Severitate: %d\n",
                                   temp.id, temp.category, temp.inspector_name, temp.severity);
                        }
                    }

                    if (match_count == 0) {
                        printf("Niciun raport nu indeplineste conditia.\n");
                    }
                } else {
                    printf("Eroare: Formatul conditiei este invalid. Foloseste camp:operator:valoare\n");
                }
            }
        }
        // Verificăm dacă e comanda update_threshold
        else if (strcmp(command, "update_threshold") == 0) {

            // 1. Doar managerii au voie să modifice pragul
            if (role == NULL || strcmp(role, "manager") != 0) {
                printf("Eroare de permisiune: Doar utilizatorii cu rolul 'manager' pot actualiza pragul!\n");
            } else {
                struct stat cfg_stat;

                // 2. Verificăm dacă fișierul există deja folosind stat()
                if (stat(config_path, &cfg_stat) == 0) {

                    // Extragem doar ultimii 9 biți (permisiunile) folosind masca 0777
                    mode_t permisiuni_curente = cfg_stat.st_mode & 0777;

                    // Verificăm dacă sunt EXACT 0640 (cerință strictă a proiectului)
                    if (permisiuni_curente != 0640) {
                        printf("Eroare de Securitate! Permisiunile pentru %s nu sunt 0640. Refuz modificarea!\n", config_path);
                    } else {
                        // Permisiunile sunt sigure, putem scrie
                        // O_TRUNC șterge textul vechi pentru a pune noua valoare
                        int fd_cfg = open(config_path, O_WRONLY | O_TRUNC);
                        char buffer[64];
                        sprintf(buffer, "SEVERITY_THRESHOLD=%d\n", threshold_val);
                        write(fd_cfg, buffer, strlen(buffer));
                        close(fd_cfg);
                        printf("Pragul de severitate a fost actualizat la %d.\n", threshold_val);
                    }
                } else {
                    // 3. Fișierul nu există, îl creăm noi curat și îi forțăm permisiunile la 0640
                    int fd_cfg = open(config_path, O_CREAT | O_WRONLY, 0640);
                    chmod(config_path, 0640); // Forțăm exact 0640

                    char buffer[64];
                    sprintf(buffer, "SEVERITY_THRESHOLD=%d\n", threshold_val);
                    write(fd_cfg, buffer, strlen(buffer));
                    close(fd_cfg);
                    printf("Fisierul de configurare a fost creat, iar pragul setat la %d.\n", threshold_val);
                }
            }
        }
        // --- CREAREA LEGĂTURII SIMBOLICE ---
        char symlink_name[256];
        sprintf(symlink_name, "active_reports-%s", district);

        // Funcția symlink(destinația, numele_shortcut_ului) creează legătura
        // Folosim unlink înainte pentru a șterge vechiul shortcut dacă există deja
        unlink(symlink_name);
        if (symlink(reports_path, symlink_name) == 0) {
            printf("Scurtatura %s a fost creata cu succes!\n", symlink_name);
        } else {
            perror("Eroare la crearea scurtaturii");
        }

        // SISTEMUL DE JURNALIZARE (LOGGING) REVIZUIT

        if (strcmp(command, "add") == 0 || strcmp(command, "remove_report") == 0 || strcmp(command, "update_threshold") == 0) {

            struct stat log_stat;
            int can_write = 0;

            // 1. Folosim stat() pentru a citi permisiunile reale ale fișierului
            if (stat(log_path, &log_stat) == 0) {
                mode_t mode = log_stat.st_mode;

                // Managerul e considerat "Owner" (Proprietar)
                if (role != NULL && strcmp(role, "manager") == 0) {
                    if (mode & S_IWUSR) can_write = 1; // Are bitul de scriere (w) pentru user?
                }
                // Inspectorul e considerat "Group" (Grup)
                else if (role != NULL && strcmp(role, "inspector") == 0) {
                    if (mode & S_IWGRP) can_write = 1; // Are bitul de scriere (w) pentru grup?
                }
            } else {
                // Dacă fișierul nu există încă, doar managerul are dreptul să inițieze crearea lui
                if (role != NULL && strcmp(role, "manager") == 0) {
                    can_write = 1;
                }
            }

            // 2. Acționăm pe baza permisiunilor citite de pe disc
            if (can_write) {
                int fd_log = open(log_path, O_CREAT | O_WRONLY | O_APPEND, 0644);
                if (fd_log != -1) {
                    chmod(log_path, 0644); // Forțăm 0644

                    char log_buffer[256];
                    time_t now = time(NULL);
// Formatăm linia de bază (fără \n la final)
                    sprintf(log_buffer, "%ld\t%s\t%s\t%s", now, user ? user : "Necunoscut", role, command);

                    // Adăugăm mesajul despre monitor DOAR dacă a fost comanda "add"
                    if (strcmp(command, "add") == 0) {
                        if (monitor_status == 1) {
                            strcat(log_buffer, "\t[Monitor Notificat Succes]\n");
                        } else {
                            strcat(log_buffer, "\t[Eroare: Monitorul NU a putut fi notificat]\n");
                        }
                    } else {
                        strcat(log_buffer, "\n"); // Pentru restul comenzilor punem doar enter la final
                    }                    write(fd_log, log_buffer, strlen(log_buffer));
                    close(fd_log);
                }
            } else {
                // Deoarece fișierul are 644 (rw-r--r--), inspectorul nu are S_IWGRP, deci pică aici automat!
                printf("Notificari Securitate: Rolul tau nu are permisiuni de scriere (lipseste bitul w) pentru %s. Actiunea nu a fost logata.\n", log_path);
            }
        }
        close(fd_reports);
    } else {
        printf("Eroare: Nu ai specificat un district în comanda!\n");
        return 1; // Ieșim din program cu eroare
    }
    return 0;
}
