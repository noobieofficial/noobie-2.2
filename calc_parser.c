#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "calc_parser.h"

// Inizializza il tokenizer
void init_tokenizer(Tokenizer *tokenizer, const char *input) {
    tokenizer->input = input;
    tokenizer->pos = 0;
    tokenizer->length = strlen(input);
    tokenizer->current_token.type = TOKEN_ERROR;
    tokenizer->has_current = false;
}

// Salta gli spazi bianchi
void skip_whitespace(Tokenizer *tokenizer) {
    while (tokenizer->pos < tokenizer->length && isspace(tokenizer->input[tokenizer->pos])) {
        tokenizer->pos++;
    }
}

// Controlla se un carattere è alfanumerico o underscore
bool is_alpha_or_underscore(char c) {
    return isalpha(c) || c == '_';
}

bool is_alnum_or_underscore(char c) {
    return isalnum(c) || c == '_';
}

// Peek del prossimo token senza consumarlo
Token peek_token(Tokenizer *tokenizer) {
    if (tokenizer->has_current) {
        return tokenizer->current_token;
    }
    
    size_t saved_pos = tokenizer->pos;
    Token token = get_next_token_internal(tokenizer);
    tokenizer->pos = saved_pos;
    return token;
}

// Consuma il token corrente se corrisponde al tipo atteso
bool consume_token(Tokenizer *tokenizer, TokenType expected_type) {
    Token token = get_next_token(tokenizer);
    return token.type == expected_type;
}

// Implementazione interna per ottenere il prossimo token
Token get_next_token_internal(Tokenizer *tokenizer) {
    Token token = {TOKEN_ERROR, "", 0.0};
    
    skip_whitespace(tokenizer);
    
    if (tokenizer->pos >= tokenizer->length) {
        token.type = TOKEN_END;
        return token;
    }
    
    char current = tokenizer->input[tokenizer->pos];
    
    // Parentesi
    if (current == '(') {
        token.type = TOKEN_LPAREN;
        token.value[0] = current;
        token.value[1] = '\0';
        tokenizer->pos++;
        return token;
    }
    
    if (current == ')') {
        token.type = TOKEN_RPAREN;
        token.value[0] = current;
        token.value[1] = '\0';
        tokenizer->pos++;
        return token;
    }
    
    // Numeri (interi e decimali)
    if (isdigit(current) || (current == '.' && tokenizer->pos + 1 < tokenizer->length && isdigit(tokenizer->input[tokenizer->pos + 1]))) {
        size_t start = tokenizer->pos;
        bool has_dot = false;
        
        while (tokenizer->pos < tokenizer->length && 
               (isdigit(tokenizer->input[tokenizer->pos]) || 
                (tokenizer->input[tokenizer->pos] == '.' && !has_dot))) {
            if (tokenizer->input[tokenizer->pos] == '.') {
                has_dot = true;
            }
            tokenizer->pos++;
        }
        
        size_t length = tokenizer->pos - start;
        strncpy(token.value, &tokenizer->input[start], length);
        token.value[length] = '\0';
        token.number = atof(token.value);
        token.type = TOKEN_NUMBER;
        return token;
    }
    
    // Operatori multicarattere
    if (tokenizer->pos + 2 < tokenizer->length && 
        strncmp(&tokenizer->input[tokenizer->pos], "***", 3) == 0) {
        strcpy(token.value, "***");
        token.type = TOKEN_OPERATOR;
        tokenizer->pos += 3;
        return token;
    }
    
    if (tokenizer->pos + 1 < tokenizer->length) {
        char two_char[3] = {current, tokenizer->input[tokenizer->pos + 1], '\0'};
        if (strcmp(two_char, "**") == 0 || strcmp(two_char, "==") == 0 || 
            strcmp(two_char, "!=") == 0 || strcmp(two_char, "<=") == 0 || 
            strcmp(two_char, ">=") == 0) {
            strcpy(token.value, two_char);
            token.type = TOKEN_OPERATOR;
            tokenizer->pos += 2;
            return token;
        }
    }
    
    // Operatori singoli
    if (current == '+' || current == '-' || current == '*' || current == '/' || 
        current == '%' || current == '<' || current == '>') {
        token.value[0] = current;
        token.value[1] = '\0';
        token.type = TOKEN_OPERATOR;
        tokenizer->pos++;
        return token;
    }
    
    // Identificatori (variabili, operatori logici, booleani)
    if (is_alpha_or_underscore(current)) {
        size_t start = tokenizer->pos;
        
        while (tokenizer->pos < tokenizer->length && 
            is_alnum_or_underscore(tokenizer->input[tokenizer->pos])) {
            tokenizer->pos++;
        }
        
        size_t length = tokenizer->pos - start;
        strncpy(token.value, &tokenizer->input[start], length);
        token.value[length] = '\0';
        
        // Converti in maiuscolo per il confronto
        char upper_value[256];
        for (int i = 0; i < length && i < 255; i++) {
            upper_value[i] = toupper(token.value[i]);
        }
        upper_value[length] = '\0';
        
        if (strcmp(upper_value, "TRUE") == 0) {
            token.type = TOKEN_BOOLEAN;
            token.number = 1.0;
            return token;
        } else if (strcmp(upper_value, "FALSE") == 0) {
            token.type = TOKEN_BOOLEAN;
            token.number = 0.0;
            return token;
        } else if (strcmp(upper_value, "AND") == 0 || strcmp(upper_value, "OR") == 0 || 
                strcmp(upper_value, "XOR") == 0 || strcmp(upper_value, "NOT") == 0) {
            // Mantieni la versione originale per gli operatori
            strcpy(token.value, upper_value);
            token.type = TOKEN_OPERATOR;
        } else {
            token.type = TOKEN_VARIABLE;
        }
        return token;
    }
    
    // Token non riconosciuto
    tokenizer->pos++;
    return token;
}

// Ottiene il prossimo token con caching
Token get_next_token(Tokenizer *tokenizer) {
    if (tokenizer->has_current) {
        tokenizer->has_current = false;
        return tokenizer->current_token;
    }
    
    Token token = get_next_token_internal(tokenizer);
    return token;
}

// Rimette un token indietro
void unget_token(Tokenizer *tokenizer, Token token) {
    tokenizer->current_token = token;
    tokenizer->has_current = true;
}

// Applica operatori binari con gestione migliorata dei tipi
CalcResult apply_binary_operator(const char *op, CalcResult left, CalcResult right, int line_number) {
    CalcResult result = {TYPE_INT, {0}};
    
    // Operatori aritmetici
    if (strcmp(op, "+") == 0) {
        if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
            result.type = TYPE_FLOAT;
            result.value.f_val = (left.type == TYPE_FLOAT ? left.value.f_val : (float)left.value.i_val) +
                               (right.type == TYPE_FLOAT ? right.value.f_val : (float)right.value.i_val);
        } else {
            result.type = TYPE_INT;
            result.value.i_val = left.value.i_val + right.value.i_val;
        }
    }
    else if (strcmp(op, "-") == 0) {
        if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
            result.type = TYPE_FLOAT;
            result.value.f_val = (left.type == TYPE_FLOAT ? left.value.f_val : (float)left.value.i_val) -
                               (right.type == TYPE_FLOAT ? right.value.f_val : (float)right.value.i_val);
        } else {
            result.type = TYPE_INT;
            result.value.i_val = left.value.i_val - right.value.i_val;
        }
    }
    else if (strcmp(op, "*") == 0) {
        if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
            result.type = TYPE_FLOAT;
            result.value.f_val = (left.type == TYPE_FLOAT ? left.value.f_val : (float)left.value.i_val) *
                               (right.type == TYPE_FLOAT ? right.value.f_val : (float)right.value.i_val);
        } else {
            result.type = TYPE_INT;
            result.value.i_val = left.value.i_val * right.value.i_val;
        }
    }
    else if (strcmp(op, "/") == 0) {
        double right_val = (right.type == TYPE_FLOAT ? right.value.f_val : (double)right.value.i_val);
        if (right_val == 0.0) {
            handle_error("DIVISION BY ZERO", line_number);
        }
        result.type = TYPE_FLOAT;
        result.value.f_val = (left.type == TYPE_FLOAT ? left.value.f_val : (float)left.value.i_val) / right_val;
    }
    else if (strcmp(op, "%") == 0) {
        if (left.type != TYPE_INT || right.type != TYPE_INT) {
            handle_error("MODULO OPERATOR REQUIRES INTEGER OPERANDS", line_number);
        }
        if (right.value.i_val == 0) {
            handle_error("MODULO BY ZERO", line_number);
        }
        result.type = TYPE_INT;
        result.value.i_val = left.value.i_val % right.value.i_val;
    }
    else if (strcmp(op, "**") == 0) {
        result.type = TYPE_FLOAT;
        double base = (left.type == TYPE_FLOAT ? left.value.f_val : (double)left.value.i_val);
        double exp = (right.type == TYPE_FLOAT ? right.value.f_val : (double)right.value.i_val);
        result.value.f_val = pow(base, exp);
    }
    else if (strcmp(op, "***") == 0) {
        result.type = TYPE_FLOAT;
        double base = (left.type == TYPE_FLOAT ? left.value.f_val : (double)left.value.i_val);
        double exp = (right.type == TYPE_FLOAT ? right.value.f_val : (double)right.value.i_val);
        if (base < 0 && exp != floor(exp)) {
            handle_error("NEGATIVE BASE WITH NON-INTEGER EXPONENT", line_number);
        }
        result.value.f_val = pow(base, exp);
    }
    // Operatori relazionali - SEMPRE restituiscono bool
    else if (strcmp(op, "==") == 0) {
        result.type = TYPE_BOOL;
        if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
            double left_val = (left.type == TYPE_FLOAT ? left.value.f_val : (double)left.value.i_val);
            double right_val = (right.type == TYPE_FLOAT ? right.value.f_val : (double)right.value.i_val);
            result.value.b_val = (fabs(left_val - right_val) < 1e-9);
        } else if (left.type == TYPE_BOOL && right.type == TYPE_BOOL) {
            result.value.b_val = (left.value.b_val == right.value.b_val);
        } else {
            result.value.b_val = (left.value.i_val == right.value.i_val);
        }
    }
    else if (strcmp(op, "!=") == 0) {
        result.type = TYPE_BOOL;
        if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
            double left_val = (left.type == TYPE_FLOAT ? left.value.f_val : (double)left.value.i_val);
            double right_val = (right.type == TYPE_FLOAT ? right.value.f_val : (double)right.value.i_val);
            result.value.b_val = (fabs(left_val - right_val) >= 1e-9);
        } else if (left.type == TYPE_BOOL && right.type == TYPE_BOOL) {
            result.value.b_val = (left.value.b_val != right.value.b_val);
        } else {
            result.value.b_val = (left.value.i_val != right.value.i_val);
        }
    }
    else if (strcmp(op, "<") == 0) {
        result.type = TYPE_BOOL;
        if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
            double left_val = (left.type == TYPE_FLOAT ? left.value.f_val : (double)left.value.i_val);
            double right_val = (right.type == TYPE_FLOAT ? right.value.f_val : (double)right.value.i_val);
            result.value.b_val = (left_val < right_val);
        } else {
            result.value.b_val = (left.value.i_val < right.value.i_val);
        }
    }
    else if (strcmp(op, ">") == 0) {
        result.type = TYPE_BOOL;
        if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
            double left_val = (left.type == TYPE_FLOAT ? left.value.f_val : (double)left.value.i_val);
            double right_val = (right.type == TYPE_FLOAT ? right.value.f_val : (double)right.value.i_val);
            result.value.b_val = (left_val > right_val);
        } else {
            result.value.b_val = (left.value.i_val > right.value.i_val);
        }
    }
    else if (strcmp(op, "<=") == 0) {
        result.type = TYPE_BOOL;
        if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
            double left_val = (left.type == TYPE_FLOAT ? left.value.f_val : (double)left.value.i_val);
            double right_val = (right.type == TYPE_FLOAT ? right.value.f_val : (double)right.value.i_val);
            result.value.b_val = (left_val <= right_val);
        } else {
            result.value.b_val = (left.value.i_val <= right.value.i_val);
        }
    }
    else if (strcmp(op, ">=") == 0) {
        result.type = TYPE_BOOL;
        if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
            double left_val = (left.type == TYPE_FLOAT ? left.value.f_val : (double)left.value.i_val);
            double right_val = (right.type == TYPE_FLOAT ? right.value.f_val : (double)right.value.i_val);
            result.value.b_val = (left_val >= right_val);
        } else {
            result.value.b_val = (left.value.i_val >= right.value.i_val);
        }
    }
    // Operatori logici
    else if (strcmp(op, "AND") == 0) {
        result.type = TYPE_BOOL;
        bool left_bool = convert_to_bool(left);
        bool right_bool = convert_to_bool(right);
        result.value.b_val = left_bool && right_bool;
    }
    else if (strcmp(op, "OR") == 0) {
        result.type = TYPE_BOOL;
        bool left_bool = convert_to_bool(left);
        bool right_bool = convert_to_bool(right);
        result.value.b_val = left_bool || right_bool;
    }
    else if (strcmp(op, "XOR") == 0) {
        result.type = TYPE_BOOL;
        bool left_bool = convert_to_bool(left);
        bool right_bool = convert_to_bool(right);
        result.value.b_val = left_bool != right_bool;
    }
    else {
        handle_error("UNKNOWN BINARY OPERATOR", line_number);
    }
    
    return result;
}

// Funzione helper per convertire qualsiasi tipo a bool
bool convert_to_bool(CalcResult value) {
    switch (value.type) {
        case TYPE_BOOL:
            return value.value.b_val;
        case TYPE_INT:
            return value.value.i_val != 0;
        case TYPE_FLOAT:
            return value.value.f_val != 0.0f;
        default:
            return false;
    }
}

// Applica operatori unari
CalcResult apply_unary_operator(const char *op, CalcResult operand, int line_number) {
    CalcResult result = operand;
    
    if (strcmp(op, "-") == 0) {
        if (operand.type == TYPE_INT) {
            result.value.i_val = -operand.value.i_val;
        } else if (operand.type == TYPE_FLOAT) {
            result.value.f_val = -operand.value.f_val;
        } else {
            handle_error("UNARY MINUS REQUIRES NUMERIC OPERAND", line_number);
        }
    }
    else if (strcmp(op, "+") == 0) {
        if (operand.type != TYPE_INT && operand.type != TYPE_FLOAT) {
            handle_error("UNARY PLUS REQUIRES NUMERIC OPERAND", line_number);
        }
    }
    else if (strcmp(op, "NOT") == 0) {
        result.type = TYPE_BOOL;
        result.value.b_val = !convert_to_bool(operand);
    }
    else {
        handle_error("UNKNOWN UNARY OPERATOR", line_number);
    }
    
    return result;
}

// Parser per espressioni primarie (numeri, variabili, parentesi)
CalcResult parse_primary_expression(Tokenizer *tokenizer, int line_number) {
    CalcResult result = {TYPE_INT, {0}};
    Token token = get_next_token(tokenizer);
    
    if (token.type == TOKEN_NUMBER) {
        if (strchr(token.value, '.') != NULL) {
            result.type = TYPE_FLOAT;
            result.value.f_val = (float)token.number;
        } else {
            result.type = TYPE_INT;
            result.value.i_val = (int)token.number;
        }
        return result;
    }

    if (token.type == TOKEN_BOOLEAN) {
        result.type = TYPE_BOOL;
        result.value.b_val = (token.number != 0.0);
        return result;
    }

    if (token.type == TOKEN_VARIABLE) {
        Variable *var = find_variable(token.value);
        if (!var) {
            handle_error("VARIABLE NOT FOUND IN EXPRESSION", line_number);
        }
        
        result.type = var->type;
        switch (var->type) {
            case TYPE_INT:
                result.value.i_val = var->value.i_val;
                break;
            case TYPE_FLOAT:
                result.value.f_val = var->value.f_val;
                break;
            case TYPE_BOOL:
                result.value.b_val = var->value.b_val;
                break;
            default:
                handle_error("UNSUPPORTED VARIABLE TYPE IN EXPRESSION", line_number);
        }
        return result;
    }
    
    if (token.type == TOKEN_LPAREN) {
        result = parse_expression(tokenizer, line_number);
        token = get_next_token(tokenizer);
        if (token.type != TOKEN_RPAREN) {
            handle_error("MISSING CLOSING PARENTHESIS", line_number);
        }
        return result;
    }
    
    handle_error("INVALID PRIMARY EXPRESSION", line_number);
    return result;
}

// Parser per espressioni unarie
CalcResult parse_unary_expression(Tokenizer *tokenizer, int line_number) {
    Token token = get_next_token(tokenizer);
    
    if (token.type == TOKEN_OPERATOR && 
        (strcmp(token.value, "-") == 0 || strcmp(token.value, "+") == 0 || strcmp(token.value, "NOT") == 0)) {
        CalcResult operand = parse_unary_expression(tokenizer, line_number);
        return apply_unary_operator(token.value, operand, line_number);
    } else {
        // Rimetti il token indietro
        unget_token(tokenizer, token);
        return parse_primary_expression(tokenizer, line_number);
    }
}

// Parser per espressioni di potenza (associatività destra)
CalcResult parse_power_expression(Tokenizer *tokenizer, int line_number) {
    CalcResult left = parse_unary_expression(tokenizer, line_number);
    
    Token token = get_next_token(tokenizer);
    if (token.type == TOKEN_OPERATOR && 
        (strcmp(token.value, "**") == 0 || strcmp(token.value, "***") == 0)) {
        CalcResult right = parse_power_expression(tokenizer, line_number); // Associatività destra
        return apply_binary_operator(token.value, left, right, line_number);
    }
    
    unget_token(tokenizer, token);
    return left;
}

// Parser per espressioni moltiplicative
CalcResult parse_multiplicative_expression(Tokenizer *tokenizer, int line_number) {
    CalcResult left = parse_power_expression(tokenizer, line_number);
    
    Token token;
    while ((token = get_next_token(tokenizer)).type == TOKEN_OPERATOR && 
           (strcmp(token.value, "*") == 0 || strcmp(token.value, "/") == 0 || strcmp(token.value, "%") == 0)) {
        CalcResult right = parse_power_expression(tokenizer, line_number);
        left = apply_binary_operator(token.value, left, right, line_number);
    }
    
    unget_token(tokenizer, token);
    return left;
}

// Parser per espressioni additive
CalcResult parse_additive_expression(Tokenizer *tokenizer, int line_number) {
    CalcResult left = parse_multiplicative_expression(tokenizer, line_number);
    
    Token token;
    while ((token = get_next_token(tokenizer)).type == TOKEN_OPERATOR && 
           (strcmp(token.value, "+") == 0 || strcmp(token.value, "-") == 0)) {
        CalcResult right = parse_multiplicative_expression(tokenizer, line_number);
        left = apply_binary_operator(token.value, left, right, line_number);
    }
    
    unget_token(tokenizer, token);
    return left;
}

// Parser per espressioni relazionali
CalcResult parse_relational_expression(Tokenizer *tokenizer, int line_number) {
    CalcResult left = parse_additive_expression(tokenizer, line_number);
    
    Token token;
    while ((token = get_next_token(tokenizer)).type == TOKEN_OPERATOR && 
           (strcmp(token.value, "<") == 0 || strcmp(token.value, ">") == 0 || 
            strcmp(token.value, "<=") == 0 || strcmp(token.value, ">=") == 0)) {
        CalcResult right = parse_additive_expression(tokenizer, line_number);
        left = apply_binary_operator(token.value, left, right, line_number);
    }
    
    unget_token(tokenizer, token);
    return left;
}

// Parser per espressioni di uguaglianza
CalcResult parse_equality_expression(Tokenizer *tokenizer, int line_number) {
    CalcResult left = parse_relational_expression(tokenizer, line_number);
    
    Token token;
    while ((token = get_next_token(tokenizer)).type == TOKEN_OPERATOR && 
           (strcmp(token.value, "==") == 0 || strcmp(token.value, "!=") == 0)) {
        CalcResult right = parse_relational_expression(tokenizer, line_number);
        left = apply_binary_operator(token.value, left, right, line_number);
    }
    
    unget_token(tokenizer, token);
    return left;
}

// Parser per espressioni AND
CalcResult parse_and_expression(Tokenizer *tokenizer, int line_number) {
    CalcResult left = parse_equality_expression(tokenizer, line_number);
    
    Token token;
    while ((token = get_next_token(tokenizer)).type == TOKEN_OPERATOR && strcmp(token.value, "AND") == 0) {
        CalcResult right = parse_equality_expression(tokenizer, line_number);
        left = apply_binary_operator(token.value, left, right, line_number);
    }
    
    unget_token(tokenizer, token);
    return left;
}

// Parser per espressioni XOR
CalcResult parse_xor_expression(Tokenizer *tokenizer, int line_number) {
    CalcResult left = parse_and_expression(tokenizer, line_number);
    
    Token token;
    while ((token = get_next_token(tokenizer)).type == TOKEN_OPERATOR && strcmp(token.value, "XOR") == 0) {
        CalcResult right = parse_and_expression(tokenizer, line_number);
        left = apply_binary_operator(token.value, left, right, line_number);
    }
    
    unget_token(tokenizer, token);
    return left;
}

// Parser per espressioni OR
CalcResult parse_or_expression(Tokenizer *tokenizer, int line_number) {
    CalcResult left = parse_xor_expression(tokenizer, line_number);
    
    Token token;
    while ((token = get_next_token(tokenizer)).type == TOKEN_OPERATOR && strcmp(token.value, "OR") == 0) {
        CalcResult right = parse_xor_expression(tokenizer, line_number);
        left = apply_binary_operator(token.value, left, right, line_number);
    }
    
    unget_token(tokenizer, token);
    return left;
}

// Parser principale per espressioni
CalcResult parse_expression(Tokenizer *tokenizer, int line_number) {
    return parse_or_expression(tokenizer, line_number);
}

// Funzione principale per valutare un'espressione
CalcResult evaluate_expression(const char *expression, int line_number) {
    Tokenizer tokenizer;
    init_tokenizer(&tokenizer, expression);
    
    CalcResult result = parse_expression(&tokenizer, line_number);
    
    // Verifica che l'espressione sia completamente consumata
    Token token = get_next_token(&tokenizer);
    if (token.type != TOKEN_END) {
        handle_error("UNEXPECTED TOKEN IN EXPRESSION", line_number);
    }
    
    return result;
}

// Funzioni di utilità rimosse dal codice originale ma mantenute per compatibilità
CalcResult convert_to_common_type(CalcResult a, CalcResult b) {
    // Questa funzione non è più utilizzata con la nuova implementazione
    // ma mantenuta per compatibilità
    CalcResult result = {TYPE_INT, {0}};
    
    if (a.type == TYPE_FLOAT || b.type == TYPE_FLOAT) {
        result.type = TYPE_FLOAT;
        result.value.f_val = (a.type == TYPE_FLOAT ? a.value.f_val : (float)a.value.i_val) +
                           (b.type == TYPE_FLOAT ? b.value.f_val : (float)b.value.i_val);
    } else {
        result.type = TYPE_INT;
        result.value.i_val = a.value.i_val + b.value.i_val;
    }
    
    return result;
}

bool is_operator(const char *str) {
    const char *operators[] = {
        "***", "**", "+", "-", "*", "/", "%",
        "AND", "OR", "XOR", "NOT",
        "==", "!=", "<=", ">=", "<", ">"
    };
    int num_operators = sizeof(operators) / sizeof(operators[0]);
    
    for (int i = 0; i < num_operators; i++) {
        if (strcmp(str, operators[i]) == 0) {
            return true;
        }
    }
    return false;
}
