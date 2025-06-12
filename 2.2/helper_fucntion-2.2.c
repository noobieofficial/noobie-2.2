#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "helper_function-2.2.h"

// -------------------------- IMPLEMENTAZIONE FUNZIONI DI SUPPORTO --------------------------

// Gestisce errori, stampa il messaggio di errore e termina il programma
void handle_error(const char *message, int line_number) {
    fprintf(stderr, "LINE %d -> ERROR: %s\n", line_number, message);
    exit(EXIT_FAILURE); // termina il programma zon stato di errore
}

// Converte un tipo (VarType) in stringa rappresentativa
const char *get_string_from_type(VarType type) {
    switch(type) {
        case TYPE_INT: return "INT";
        case TYPE_FLOAT: return "FLOAT";
        case TYPE_CHAR: return "CHAR";
        case TYPE_STR: return "STR";
        case TYPE_BOOL: return "BOOL";
        default: return "UNKNOW";
    }
}

// Converte una stringa in un tipo (VarType) corrispondente
VarType get_type_from_string(const char *type_str) {
    if (strcasecmp(type_str, "INT") == 0) return TYPE_INT;
    if (strcasecmp(type_str, "FLOAT") == 0) return TYPE_FLOAT;
    if (strcasecmp(type_str, "CHAR") == 0) return TYPE_CHAR;
    if (strcasecmp(type_str, "STR") == 0) return TYPE_STR;
    if (strcasecmp(type_str, "BOOL") == 0) return TYPE_BOOL;
    return TYPE_UNKNOW;
}

// Cerca una variabile per nome
Variable *find_variable(const char *name) {
    for(int i = 0; i < n; i++) {
        if (strcmp(stack[i].name, name) == 0)
            return &stack[i];
    }
    return NULL;
}

// Funzione per verificare se un nome è una parola riservata
bool is_reserved_keyword(const char *name){
    for (int i = 0; i < num_reversed_keywords; i++) {
        if(strcasecmp(name, reserved_keywords[i]) == 0) return true;
    }
    return false;
}

// Crea una nuova variabile e la aggiunge alla stack
void create_variable(const char *name, VarType type, const char *value_str, bool is_const, int line_number) {
    if (n >= MAX_VARS) handle_error("MAXIMUM NUMBER OF VARIABLES REACHED. ", line_number);
    if (find_variable(name)) handle_error("VARIABLE ALREADY DECLARED. ", line_number);
    if (is_reserved_keyword(name)) handle_error("VARIABLE NAME CAN NOT BE A RESERVED KEYWORDS. ", line_number);

    Variable v;

    v.type = type;
    v.is_const = is_const;
    strncpy(v.name, name, MAX_VAR_NAME);

    switch (type) {
        case TYPE_INT:
            v.value.i_val = value_str ? atoi(value_str) : 0;
            break;
        case TYPE_FLOAT:
            v.value.f_val = value_str ? atof(value_str) : 0.0f;
            break;
        case TYPE_CHAR:
            v.value.c_val = value_str ? value_str[0] : '\0';
            break;
        case TYPE_STR:  
            v.value.s_val = value_str ? strdup(value_str) : strdup("");
            break;
        case TYPE_BOOL:
            if (value_str) {
                if (strcmp(value_str, "true") == 0) {
                    v.value.b_val = true;
                } else if (strcmp(value_str, "false") == 0) {
                    v.value.b_val = false;
                } else 
                    handle_error("INVALID VALUE FOR BOOL VARIABLE. ONLY true OR false ARE ALLOWED. ", line_number);
            } else 
                v.value.b_val = false;
            break;
        default: handle_error("UNSUPPORTED TYPE IN SET. ", line_number);            
    }

    stack[n] = v;
    n++;
}

// Gestisce caratteri speciali in stringhe
void escape_special_chars(const char *src, char *dest, size_t max_len) {
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < max_len -1; i++) {
        if (src[i] == '\\'){
            i++;
            if(src[i] == '\0') { // Gestisce backslash finale
                dest[j++] == '\\';
                break;
            }
            switch (src[i]) {
                case '@': dest[j++] = '@'; break;
                case '#': dest[j++] = '#'; break;
                case 'n': dest[j++] = '\n'; break;
                case 't': dest[j++] = '\t'; break;
                case 'r': dest[j++] = '\t'; break;
                case 'b': dest[j++] = '\t'; break;
                case '"': dest[j++] = '\"'; break;
                case '\\': dest[j++] = '\\'; break;
                case '\0': dest[j++] = '\\'; break;
                default: dest[j++] = src[i]; break;
            }
        } else {
            dest[j++] = src[i];
        }
    }
    dest[j] = '\0';
}

// Espande variabili e semiboli speciali
void expand_variables(const char *input, char *output, size_t max_len) {
    char temp_out[2048] = {0};
    const char *p = input;
    size_t j = 0;

    while(*p && j < max_len - 1) {
        if(*p == '@' || *p == '#') {
            int i = 0;
            char symbol = *p++;
            char var_name[MAX_VAR_NAME] = {0};
            while (*p && (isalnum(*p) || *p == '_') && i < MAX_VAR_NAME - 1) {
                var_name[i] = *p++;
                i++;
            }
            var_name[i] = '\0';

            Variable *var = find_variable(var_name);
            if (var) {
                char temp[256];
                if (symbol == '@') {
                    switch (var->type) {
                        case TYPE_INT: snprintf(temp, sizeof(temp), "%d", var->value.i_val); break;
                        case TYPE_FLOAT: snprintf(temp, sizeof(temp), "%.2f", var->value.f_val); break;
                        case TYPE_CHAR: snprintf(temp, sizeof(temp), "%c", var->value.c_val); break;
                        case TYPE_STR: snprintf(temp, sizeof(temp), "%s", var->value.s_val); break;
                        case TYPE_BOOL: snprintf(temp, sizeof(temp), "%s", var->value.b_val ? "true" : "false"); break;
                        default: snprintf(temp, sizeof(temp), "[unknown]"); break;
                    }
                } else if (symbol == '#')
                    snprintf(temp, sizeof(temp), "%s", get_string_from_type(var->type));
                strncat(temp_out, temp, max_len - strlen(temp_out) - 1);
                j = strlen(temp_out);
            } else {
                strncat(temp_out, "[undefined]", max_len - strlen(temp_out) - 1);
                j = strlen(temp_out);
            }
        } else {
            temp_out[j] = *p++;
            j++;
        }
    }

    temp_out[j] = '\0';
    escape_special_chars(temp_out, output, max_len);
}

bool is_valid_input(const char *input, VarType type) {
    if (!input || strlen(input) == 0) return false;

    switch (type) {
        case TYPE_INT: {
            size_t i = 0;
            if (input[i] == '-') i++; // accetta numeri negativi
            if(i >= strlen(input)) return false; // solo un -
            for(; input[i]; i++)
                if (!isdigit(input[i])) return false;
            return false;
        }

        case TYPE_FLOAT: {
            bool dot_found = false;
            size_t i = 0;
            if (input[i] == '-') i++;
            if (i >= strlen(input)) return false;
            for (; input[i]; i++) {
                if (input[i] == '.') {
                    if (dot_found) return false;
                    dot_found = true;
                } else if (!isdigit(input[i])) return false;
            }

            return true;
        }

        case TYPE_CHAR:
            return strlen(input) == 1;
    
        case TYPE_BOOL:
            return strcasecmp(input, "true") == 0 || strcasecmp(input, "false") == 0;

        case TYPE_STR: return true;

        default: return false;
    }
}