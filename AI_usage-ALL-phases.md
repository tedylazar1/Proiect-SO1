# Raport privind Utilizarea Instrumentelor de Inteligenta Artificiala (AI)
## Proiect la Disciplina: Sisteme de Operare (SO)
## Autor: Lazar Theodor
## Instrument utilizat: Google Gemini

---

## INTRODUCERE
Acest document descrie modul in care instrumentul AI Google Gemini a fost utilizat ca suport in dezvoltarea si optimizarea proiectului pe parcursul celor trei faze de implementare. Utilizarea AI-ului s-a concentrat pe intelegerea conceptelor teoretice de nivel POSIX/UNIX, generarea de schelete de cod si rezolvarea unor probleme complexe legate de alinierea structurilor binare si comunicarea inter-procese.

---

## FAZA 1: FILTRAREA RAPOARTELOR SI I/O BINAR

### 1. Prompt utilizat
> "Am o structura Report in C cu campurile int severity, char category[16], char inspector_name[32]. Scrie-mi o functie care sparge un string de tipul 'camp:operator:valoare' si inca o functie care verifica daca un raport se potriveste cu acea conditie extrasa."

### 2. Ce a fost generat de AI
AI-ul a furnizat o abordare modulara alcatuita din doua componente principale:
* **Functia `parse_condition`**: Utilizeaza `sscanf` cu expresii regulate de formatare (masca `%[^:]`) pentru a extrage automat cele trei subseturi de date separate prin caracterul doua puncte (`:`), evitand astfel utilizarea repetata si nesigura a functiei `strtok`.
* **Functia `match_condition`**: Primeste structura raportului si conditia parsata, realizand conversii dinamice prin `atoi` pentru campurile numerice (`severity`) si comparatii de siruri prin `strcmp` pentru campurile text (`category` si `inspector_name`).

### 3. Integrare si contributie personala
Desi functiile logice au fost oferite de AI, integrarea lor in fluxul de I/O binar a fost realizata in totalitate de mine. Am implementat:
* Logica de citire secventiala din linia de comanda a argumentelor si parsarea lor in bucla principala.
* Pozitionarea cursorului in fisierul binar folosind apelul de sistem `lseek(fd_reports, 0, SEEK_SET)` pentru a asigura resetarea citirii la fiecare operatiune.
* Implementarea buclei `while (read(fd_reports, &r, sizeof(Report)) == sizeof(Report))` pentru procesarea eficienta, bloc cu bloc, a fisierului de date.

### 4. Ce am invatat in aceasta faza
* **Parsare avansata cu `sscanf`**: Am inteles cum masca `%[^:]` functioneaza ca o expresie regulata in C, citind date pana la intalnirea delimitatorului `:`. Aceasta metoda este mult mai sigura impotriva buffer overflow-ului fata de alte functii de manipulare a sirurilor.
* **Separarea conceptelor (Separation of Concerns)**: Am invatat cat de important este sa separi logica pur matematica de filtrare a datelor de logica de acces la nivel de disc (citirea de octeti din fisiere), facilitand depanarea ulterioara a aplicatiei.

---

## FAZA 2: GESTIUNEA PROCESELOR SI SEMNALELOR UNIX

### 1. Prompturi utilizate
> "Cum pot sa sterg un director si un symlink in C folosind un proces copil separat prin fork si exec? Nu am voie sa folosesc functia system()."
> "Scrie-mi un program separat in C numit monitor_reports care sa ruleze la infinit, sa salveze PID-ul propriu intr-un fisier ascuns .monitor_pid si sa asculte semnalele SIGUSR1 si SIGINT folosind structura sigaction, deoarece functia signal() ne este interzisa."

### 2. Ce a fost generat de AI
* Pentru comanda de stergere, AI-ul a generat un model bazat pe combinatia standard `fork()`, `execlp("rm", "rm", "-rf", district, NULL)` si mecanismul de sincronizare `wait(NULL)` in procesul parinte.
* Pentru monitor, AI-ul a generat un schelet de aplicatie care configureaza `struct sigaction`, defineste un handler asincron ce modifica variabile de tip `volatile sig_atomic_t` si foloseste apelul `pause()` pentru optimizarea consumului de procesor.

### 3. Integrare si contributie personala
* Am integrat stergerea districtului in `city_manager.c`, adaugand validari stricte de securitate (doar utilizatorii cu rolul de "manager" au dreptul de a declansa procesul de stergere).
* Am adaugat functionalitatea ca parintele, dupa ce `wait(NULL)` confirma terminarea executiei utilitarului `rm`, sa stearga si scurtatura prin apelul de sistem `unlink()`.
* Am modificat sistemul de jurnalizare revizuit astfel incat sa scrie in logul districtului daca notificarea catre procesul monitor a reusit sau a esuat, utilizand functia `kill(monitor_pid, SIGUSR1)`.

### 4. Ce am invatat in aceasta faza
* **De ce este `signal()` invechita**: Am inteles ca functia `signal()` are un comportament nesigur in sisteme UNIX moderne deoarece reseteaza handlerul dupa prima rulare si nu blocheaza alte semnale in timpul executiei. `sigaction()` ofera un control stabil si robust.
* **Siguranta in Handlere (Async-Signal-Safety)**: Am invatat ca functii precum `printf()` nu trebuie rulate in interiorul unui handler de semnal deoarece nu sunt sigure (pot bloca aplicatia daca semnalul vine in timpul unui alt `printf`). Solutia corecta implementata a fost modificarea unui flag atomic, procesarea textului facandu-se in `main`.
* **Eficienta resurselor cu `pause()`**: In loc de o bucla vida `while(1)` care ar fi blocat procesorul la 100%, apelul `pause()` suspenda procesul pana la venirea unui semnal, reducand consumul CPU la 0%.

---

## FAZA 3: MENIUL CENTRALIZAT SI COMUNICAREA PRIN PIPES

### 1. Prompturi utilizate
> "Vreau sa fac un program principal numit city_hub care sa functioneze ca un meniu. Cand tastez start_monitor, sa porneasca procesul monitor_reports pe fundal si sa capteze tot ce printeaza el pe ecran printr-un pipe. Cand tastez calculate_scores, sa porneasca procese paralele scorer pentru districte."
> "Programul meu scorer citeste date complet decalate din fisierul binar, in loc de scor imi apar numere uriase precum timestamp-ul. Cum repar eroarea?"

### 2. Ce a fost generat de AI
* AI-ul a generat structura meniului iterativ din `city_hub.c`, demonstrand cum se creeaza un canal anonim prin `pipe(pipefd)`, cum se realizeaza clonarea prin `fork()` si cum se redirecteaza `STDOUT_FILENO` si `STDERR_FILENO` folosind apelul `dup2()`.
* Pentru rezolvarea erorii de calcul din `scorer.c`, AI-ul a explicat teoretic fenomenul de decalaj la citirea binara si a furnizat corectia alignment-ului structurii.

### 3. Integrare si contributie personala
* Am adaptat citirea datelor din pipe in `city_hub` in mod non-blocant prin flag-ul `O_NONBLOCK` setat cu `fcntl()`, prevenind blocarea meniului principal atunci cand monitorul intra in starea de asteptare.
* Am corectat structura `Report` din fișierul `scorer.c` pentru a se potrivi la nivel de octet cu cea din `city_manager.c`, inlocuind campurile generice cu cele exacte (id, inspector_name, latitude, longitude, category, severity, timestamp, description).
* Am corectat logica din `scorer.c` transformand variabila interna `r.user` in `r.inspector_name` pentru a mapa perfect datele si a compila aplicatia fara erori.

### 4. Ce am invatat in aceasta faza
* **Mecanismul de Pipes si Redirectari (`dup2`)**: Am inteles in profunzime cum descriptorii de fisiere (`0`, `1`, `2`) pot fi redirectionati la nivel de kernel. Folosind `dup2(pipefd[1], 1)`, tot ce scrie procesul copil prin `printf` ajunge in conducta parintelui, permitand agregarea informatiilor.
* **Alinierea datelor in fisiere binare (Struct Padding & Alignment)**: Am invatat o lectie esentiala de programare low-level: fisierele binare nu contin delimitatori. Daca structura folosita la citire nu are exact aceleasi tipuri de date si aceeasi ordine ca cea folosita la scriere, pointerul de citire se decaleaza, interpretand text ca intregi sau invers (cauza numerelor uriase din timestamp). Alinierea perfecta a structurilor a rezolvat problema.

---

## CONCLUZII
Utilizarea instrumentului Google Gemini a reprezentat un factor major in accelerarea procesului de invatare. AI-ul nu a fost folosit ca un simplu generator de cod "copy-paste", ci ca un partener de debugging si un tutore teoretic care a explicat mecanismele interne ale nucleului Linux (procese, semnale, descriptori si memorie binară), ajutandu-ma sa finalizez un proiect robust, sigur si complet conform cerintelor POSIX.