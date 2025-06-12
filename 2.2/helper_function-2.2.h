#ifndef HELPER_FUNCTION_H
#define HELPER_FUNCTION_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Definizione condivise con il main
#define MAX_VAR_NAME 64
#define MAX_VARS 128

// Dichirazione dei tipi
typedef enum {
    TYPE_INT, TYPE_FLOAT, TYPE_CHAR, TYPE_STR, TYPE_BOOL, TYPE_UNKNOW
} VarType;

// Dichiarazione della struttura
typedef struct Variable {
    char name[MAX_VAR_NAME];
    VarType type;
    union {
        int i_val;
        float f_val;
        char c_val;
        char *s_val;
        bool b_val;
    } value;
    bool is_const;
} Variable;

// DIchiarazione delle variabili globali defnite nel main
extern int n;
extern Variable stack[MAX_VARS];
extern const char *reserved_keywords[];
extern const int num_reversed_keywords;

// Dichiarazione delle funzioni di supporto
Variable *find_variable(const char *name);
bool is_reserved_keyword(const char *name);
const char *get_string_from_type(VarType type);
VarType get_type_from_string(const char *type_str);
bool is_valid_input(const char *input, VarType type);
void handle_error(const char *message, int line_number);
void escape_special_chars(const char *src, char *dest, size_t max_len);
void expand_variables(const char *input, char *output, size_t max_len);
void create_variables(const char *name, VarType type, const char *value_str, bool is_const, int line_number);

#endif