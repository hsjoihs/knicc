#include <ctype.h> /* isalpha, isblank, isdigit */

#ifndef TOKEN_H
#include "token.h"
#endif

#ifndef LEXER_H
#define LEXER_H

typedef struct {
    char src[1000];
    int index;
    Token tokens[1000];
    int token_index;
} Lexer;

Token lex(Lexer *l) {
    Token t;
    int i = 0;
    char c = l->src[l->index];
    if (isdigit(c)) {
        while(isdigit(c)) {
            t.literal[i] = c;
            i++;
            l->index += 1;
            c = l->src[l->index];
        }
        t.literal[i] = '\0';
        t.token_type = INT;
    } else if (isalpha(c)) {
        while(isdigit(c) || isalpha(c) || c == '_') {
            t.literal[i] = c;
            i++;
            l->index += 1;
            c = l->src[l->index];
        }
        t.literal[i] = '\0';
        t.token_type = IDENT;
    } else if (c == '=') {
        t.literal[0] = '=';
        t.literal[1] = '\0';
        t.token_type = ASSIGN;
        l->index += 1;   
    }else if (c == '+') {
        t.literal[0] = '+';
        t.literal[1] = '\0';
        t.token_type = ADD;
        l->index += 1;
    } else if (c == '-') {
        t.literal[0] = '-';
        t.literal[1] = '\0';
        t.token_type = SUB;
        l->index += 1;
    } else if (isblank(c)) {
        l->index += 1;
        return lex(l);
    } else {
        t.literal[0] = '\0';
        t.token_type = _EOF;
    }
    return t;
}

void store_token(Lexer *l, Token t) {
    l->tokens[l->token_index] = t;
    l->token_index += 1;
}

Lexer init_lexer() {
    Lexer l;
    l.index = 0;
    l.token_index = 0;
    return l;
}

Token get_token(Lexer *l) {
    Token t = l->tokens[l->token_index];
    l->token_index += 1;
    return t;
}

Token peek_token(Lexer *l) {
    return l->tokens[l->token_index];
}

#endif