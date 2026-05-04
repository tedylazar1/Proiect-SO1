Tool folosit: Google Gemini

Prompt folosit: "Am o structura Report in C cu campurile int severity, char category[16], char inspector_name[32]. Scrie-mi o functie care sparge un string de tipul 'camp:operator:valoare' si inca o functie care verifica daca un raport se potriveste cu acea conditie extrasa."

Ce a fost generat: AI-ul a generat functia parse_condition folosind sscanf (pentru extragerea cuvintelor separate prin :) si functia match_condition care foloseste strcmp si atoi pentru a compara datele text cu cele din structura mea.

Ce am modificat / integrat eu: Nu am modificat functiile in sine, dar am scris logica de integrare in functia main. Am adaugat citirea din linia de comanda, am mutat cursorul la inceputul fisierului binar cu lseek(fd_reports, 0, SEEK_SET) si am aplicat functiile AI-ului intr-o bucla while care citeste rapoartele bloc cu bloc.

Ce am invatat: Am invatat cum se poate folosi sscanf in mod avansat (%[^:]) pentru a parsa string-uri dupa un delimitator specific si cum sa separ logica pur matematica/de filtrare de logica de acces la fisiere (I/O).

Am folosit gemini si pentru faza 2 ca sa inteleg cum functioneaza sigaction in loc de signal si cum sa scriu logica pentru monitor_reports