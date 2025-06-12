#include <ctype.h> // Per funzioni di utilità
#include <stdio.h> // Per gestire gli stream
#include <stdlib.h> // Per funzioni di utilità
#include <string.h> // Per manipolazione delle stringhe
#include <stdbool.h> // Per supporto al tipo booleano

#include "helper_function-2.2.h" // Header con funzioni personalizzate

#define MAX_VARS 128
#define MAX_LINE_LENGTH 1024 

Variable stack[MAX_VARS]; // Array di variabili chr agisce come una stack
int n = 0; // Contatore delle variabili dichiarate

// Elenco delle parole chiave riservate usate dal linguaggio
const char *reserved_keywords[] = {
    "INT", "FLOAT", "CHAR", "STR", "BOOL",
    "SET", "CONST", "SAY", "LISTEN", "EXIT", "LINE", "CLEAR"
};

// Numero delle parole chiave riservate
const int num_reserved_keywords = sizeof(reserved_keywords) / sizeof(reserved_keywords[0]);

/// ----------------- INTERPRETE -----------------
void interpret(const char *filename) {
    FILE* file = fopen(filename, "r"); // Apre il file in modalità lettura
    if (!file) handle_error("COULD NOT OPEN FILE. ", -1); // Se fallisce errore

    int n_line = 0; // Numero corrente della riga
    char line[MAX_LINE_LENGTH]; // Buffer ogni riga del file
    bool in_multiline_comment = false; // Flag per commenti multilinea

    while(fgets(line, sizeof(line), file)) { // Legge una riga per volta
        n_line++;
        line[strcspn(line, "\n")] = '\0'; // Rimuove newline finale

        // ----------- GESTIONE COMMENTI MULTILINEA ----------
        if (in_multiline_comment) {
            char *end_comment = strstr(line, ">");
            if (end_comment) { // Fine del commento multilinea
                in_multiline_comment = false;
                memmove(line, end_comment + 1, strlen(end_comment + 1) + 1);
            } else {
                continue; // Ignora tutta la riga
            }
        }

        // ----------- INIZIO/FINE COMMENTI MULTILINEA O INLINE ----------
        char *start_comment = strstr(line, "<");
        char *end_comment = strstr(line, ">");

        if (start_comment && end_comment && start_comment < end_comment) {
            // Commento chiuso nella stessa riga
            size_t start_pos = start_comment - line;
            size_t end_pos = end_comment - line + 1;
            memmove(line + start_pos, line + end_pos, strlen(line) - end_pos + 1);
        } else if (start_comment && !end_comment) {
            // Inizio di un commento multilinea
            in_multiline_comment = true;
            *start_comment = '\0'; // Tronza a inizio commento8
        }

        // ---------- GESTIONE COMMENTI DI LINEA ----------
        char *comment_start = strstr(line, "--");
        if (comment_start) *comment_start = '\0'; // Tronza a inizio commento inline;

        if (strlen(line) == 0) continue; // Salta righe vuote

        // ---------- TOKENIZZAZIONE DELLA RIGA ----------
        char *tokens[32] = { NULL };
        char line_copy[MAX_LINE_LENGTH];
        strncpy(line_copy, line, sizeof(line_copy));
        line_copy[sizeof(line_copy)-1] = '\0';
    
        int t = 0;
        char *token = strtok(line_copy, " ");
        while(token && t < 32) {
            tokens[t] = token;
            token = strtok(NULL, " ");
            t++;
        }
        if (t == 0) continue;

        // ---------- GESTIONE COMANDI ----------
        if (strcasecmp(tokens[0], "CLEAR") == 0) {
            printf("\033[H\033[J");
            continue;
        }

        else if (strcasecmp(tokens[0], "EXIT") == 0) {
            char *message = line + sizeof("EXIT");

            while (*message == ' ') message++;
            if (strlen(message) > 0) {
                if (message[0] == '"' && message[strlen(message) - 1] == '"') {
                    message[strlen(message) - 1] = '\0';
                    message++;
                }
                char expanded[1024];
                expand_variables(message, expanded, sizeof(expanded));
                printf("%s\n", expanded);
            } else 
                printf("Exiting program... Goodbye!\n");
                
            exit(0);
    }

    else if (strcasecmp(tokens[0], "LINE") == 0) {
        char expanded[512];
        char *message = line + sizeof("EXIT");

        expand_variables(message, expanded, sizeof(expanded));

        char *first = strtok(expanded, " ");
        if (!first) handle_error("LINE REQUIRES AT LEAST ONE PARAMETER. ", n_line);

        char *p = first;
        if (*p == '+') p++;
        if (*p == '-' || *p == '\0') handle_error("INVALID INTEGER VALUE FOR LINE. ", n_line);

        while (*p) {
            if (!isdigit(*p)) handle_error("INVALIDD INTEGER VALUE FOR LINE. ", n_line);
            p++;
        }
        int count = atoi(first);

        char *symbol = strtok(NULL, "");
        if (!symbol || !*symbol) symbol = "-";

        size_t len = strlen(symbol);
        for (int i = 0; i < count; i++) 
            putchar(symbol[i % len]);
        putchar('\n');
        continue;
    }

    else if (strcasecmp(tokens[0], "SET") == 0) {
        bool is_const = false;
        char type[16] = {0}, name[64] = {0}, value[256] = {0};

        if (t >= 4 && strcasecmp(tokens[1], "CONST") == 0) {
            is_const = true;
            strncpy(type, tokens[2], sizeof(type));
            strncpy(name, tokens[3], sizeof(name));
            if (t == 5) strncpy(value, tokens[4], sizeof(value));
        } else if (t >= 3) {
            strncpy(type, tokens[1], sizeof(type));
            strncpy(name, tokens[3], sizeof(name));
            if (t == 4) strncpy(value, tokens[3], sizeof(value));
        } else 
            handle_error("SET REQUIRES AT LEAST TYPE AND VARIABLE NAME. ", n_line);

        VarType vtype = get_type_from_string(type);
        if (vtype == TYPE_UNKNOW) handle_error("UNKNOW TYPE IN SET. ", n_line);

        create_variables(name, vtype, (strlen(value) > 0 ? value : NULL), is_const, n_line);
        continue;
    }

    else if (strcasecmp(tokens[0], "SAY") == 0 && t >= 2) {
        char final[1024] = {0};
        char *p = line + strlen("SAY");

        // Se troviamo una stringa tra virgolette
        if (*p == '"') {
            p++; // Salta l'inizio "
            char *start = p;
            while (*p && *p != '"') p++;
            if (*p == '\0') {
                handle_error("MISSING CLOSING QUOTE IN SAY COMMAND. ", n_line);
                break;
            }
            char temp[512];
            strncpy(temp, start, p - start);
            temp[p - start] = '\0';
            strcat(final, temp);
            p++; // Salta la chiusura
        }
        // Altrimenti, trattiamo come nome variabile
        else {
            char varname[64];
            if (sscanf(p, "%63s", varname) == 1) {
                strcat(final, "@");
                strcat(final, varname);
                p += strlen(varname);
            } else
                break; // niente da leggere
        }

        // Espandiamo
        char expanded[1024];
        expand_variables(final, expanded, sizeof(expanded));
        printf("%s", expanded);
        continue;
    }

    else if (strcasecmp(tokens[0], "LISTEN") == 0) {
        if (t < 3) handle_error("LISTEN REQUIRES AT LEAST TYPE. ", n_line);

        char type_str[16], var_name[64] = "listened", prompt_raw[512] = "";

        strncpy(type_str, tokens[1], sizeof(type_str));
        VarType type = get_type_from_string(type_str);
        if (type == TYPE_UNKNOW) handle_error("UNKNOW TYPE IN LISTEN. ", n_line);

        int prompt_index = 2;

        // Se è presente un nome variabile, lo salviamo
        if (tokens[2][0] != '"' && t >= 4) {
            strncpy(var_name, tokens[2], sizeof(var_name));
            prompt_index = 3;
        }

        // Prepara il prompt nello stesso modo del SAY
        char final_prompt[1024] = {0};
        char *p = line;
        int skip = 0;

        // Calcola l'offset in caratteri, non parole, per dove inizia il prompt
        for (int i = 0; i < prompt_index; i++)
            skip += strlen(tokens[i]) + 1;
        p += skip;

        while (*p != '\0') {
            while (*p == ' ') p++;

            if (*p == '"') {
                p++;
                const char *start = p;
                while (*p && *p != '"') p++;
                if (*p == '\0') {
                    handle_error("MISSING CLOSING QUOTE IN LISTEN PROMPT. ", n_line);
                    break;
                }
                char temp[512];
                strncpy(temp, start, p - start);
                temp[p - start] = '\0';
                strcat(final_prompt, temp);
                p++; // skippa l'ultimo "
            } else {
                char varname[64];
                if (sscanf(p, "%63s", varname) == 1) {
                    strcat(final_prompt, "@");
                    strcat(final_prompt, varname);
                    p += strlen(varname);
                } else 
                    break;
            }
        }

        // Espansone e stampa del prompt
        char expanded_prompt[512];
        expand_variables(final_prompt, expanded_prompt, sizeof(expanded_prompt));
        printf("%s", expanded_prompt);

        char input_value[256];
        if (!fgets(input_value, sizeof(input_value), stdin)) 
            handle_error("FAILED THE READ INPUT. ", n_line);
        input_value[strcspn(input_value, "\n")] = '\0';

        if(!is_valid_input(input_value, type)) handle_error("INPUT VALUES DOES NOT MATCH EXPECTED TYPE. ", n_line);

        // Liberia memoria se variabile già esisteste ed è STR
        Variable *existing = find_variable(var_name);
        if(existing && existing -> type == TYPE_STR && existing -> value.s_val) {
            free(existing->value.s_val);
        }

        create_variable(var_name, type, input_value, false, n_line);
        continue;
    }    

    else if (strcasecmp(tokens[0], "INCREMENT") == 0) {
        if (t < 2) handle_error("INCREMENT REQUIRES A VARIABLE NAME.", n_line);
            char var_name[64];
            strncpy(var_name, tokens[1], sizeof(var_name));
            Variable *var = find_variable(var_name);
            if (!var) handle_error("VARIABLE NOT FOUND.", n_line);
            if (var->type != TYPE_INT & var->type != TYPE_FLOAT) handle_error("INCREMENT ONLY WORKS WITH INTEGER AND FLOAT VARIABLES. ", n_line);
            var->type == TYPE_INT ? var->value.i_val++ : var->value.f_val++;
            continue;
        }

        else if (strcasecmp(tokens[0], "DECREMENT") == 0) {
            if (t < 2) handle_error("DECREMENT REQUIRES A VARIABLE NAME. ", n_line);
            char var_name[64];
            strncpy(var_name, tokens[1], sizeof(var_name));
            Variable *var = find_variable(var_name);
            if (!var) handle_error("VARIABLE NOT FOUND. ", n_line);
            if(var->type != TYPE_INT && var->type != TYPE_FLOAT) handle_error("DECREMENT ONLY WORKS WITH INTEGER AND FLOAT VARIABLES. ", n_line);
            var->type==TYPE_INT ? var->value.i_val-- : var->value.f_val--;
            continue;
        }
        
        else {
            char msg[256];
            snprintf(msg, sizeof(msg), "UNKNOW COMMAND: %.200s", line);
            handle_error(msg, n_line);
        }
    }

    fclose(file);
}

/// ---------- MAIN ----------
int main(int argc, char *argv[]) {
    if (argc < 2) handle_error("USAGE: ./noobie_interpreter <file.nob> ", -1);
    interpret(argv[1]);
    return 0;
}