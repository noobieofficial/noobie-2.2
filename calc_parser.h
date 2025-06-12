#ifndef CALC_PARSER_H
#define CALC_PARSER_H

#include <stdbool.h>
#include "helper_function-2_2.h"

// Tipi di token per il parser
typedef enum {
    TOKEN_NUMBER,
    TOKEN_VARIABLE,
    TOKEN_BOOLEAN,
    TOKEN_OPERATOR,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_END,
    TOKEN_ERROR
} TokenType;

// Struttura per rappresentare un token
typedef struct {
    TokenType type;
    char value[256];
    double number;
} Token;

// Struttura per il risultato di una valutazione
typedef struct {
    VarType type;
    union {
        int i_val;
        float f_val;
        bool b_val;
    } value;
} CalcResult;

// Struttura per il tokenizer
typedef struct {
    const char *input;
    size_t pos;
    size_t length;
} Tokenizer;

// Dichiarazioni delle funzioni del parser
void init_tokenizer(Tokenizer *tokenizer, const char *input);
Token get_next_token(Tokenizer *tokenizer);
CalcResult evaluate_expression(const char *expression, int line_number);
CalcResult parse_expression(Tokenizer *tokenizer, int line_number);
CalcResult parse_or_expression(Tokenizer *tokenizer, int line_number);
CalcResult parse_xor_expression(Tokenizer *tokenizer, int line_number);
CalcResult parse_and_expression(Tokenizer *tokenizer, int line_number);
CalcResult parse_equality_expression(Tokenizer *tokenizer, int line_number);
CalcResult parse_relational_expression(Tokenizer *tokenizer, int line_number);
CalcResult parse_additive_expression(Tokenizer *tokenizer, int line_number);
CalcResult parse_multiplicative_expression(Tokenizer *tokenizer, int line_number);
CalcResult parse_power_expression(Tokenizer *tokenizer, int line_number);
CalcResult parse_unary_expression(Tokenizer *tokenizer, int line_number);
CalcResult parse_primary_expression(Tokenizer *tokenizer, int line_number);

// Funzioni di utilità
bool is_operator(const char *str);
int get_operator_precedence(const char *op);
CalcResult apply_binary_operator(const char *op, CalcResult left, CalcResult right, int line_number);
CalcResult apply_unary_operator(const char *op, CalcResult operand, int line_number);
CalcResult convert_to_common_type(CalcResult a, CalcResult b);
void skip_whitespace(Tokenizer *tokenizer);
bool is_alpha_or_underscore(char c);
bool is_alnum_or_underscore(char c);

#endif
