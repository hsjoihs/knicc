#ifndef KNICC_H
#define KNICC_H

#include <stdlib.h>
#include <stdbool.h>

// token.c
typedef enum {
    INT,
    IDENT,
    SEMICOLON,
    ADD,
    SUB,
    MULTI,
    ASSIGN,
    LParen,
    RParen,
    LBrace,
    RBrace,
    COMMA,
    _EOF,
    NOT_FOUND, // used for only special_char()
    ERR, // unused

    // used only ast
    FUNC_DECL,
    FUNC_CALL,
} TokenType;

typedef struct {
    char literal[256];
    TokenType type;
} Token;

extern TokenType spacial_char(char c);
extern Token new_token(char *literal, TokenType kind);
extern bool assert_token(TokenType expected, TokenType got);
extern void debug_token(Token t);
extern bool is_binop(TokenType type);

// lexer.c
typedef struct {
    char src[1000];
    int index;
    Token tokens[1000];
    int token_index;
    int length;
} Lexer;

extern Token lex(Lexer *l);
extern void store_token(Lexer *l, Token t);
extern Lexer init_lexer();
extern Token get_token(Lexer *l);
extern Token peek_token(Lexer *l);

// map.c
typedef struct {
	char *key;
	int value;
} KeyValue;

typedef struct {
	size_t length;
	size_t buf;
	KeyValue *data; // void
} Vector;

extern Vector *init_vector(void);
extern size_t vec_size(Vector *vec);
extern void vec_push(Vector *vec, KeyValue *item);
extern void debug_vec(Vector *vec);
extern KeyValue *new_kv(char *key, int value);
extern void debug_kv(KeyValue *kv);
KeyValue *vec_get(Vector *vec, int index);
KeyValue *find_by_key(Vector *vec, char *key);

// node.c
struct CompoundStatement;
typedef struct Node {
    int type;
    int ival;
    char *literal;
    struct Node *left;
    struct Node *right;
    struct { // FUNC_CALL
        char *func_name;
        int argc;
        int *argv;
    } func_call;
    struct { // FUNC_DECL
        char *func_name;
        struct CompoundStatement *stmt;
        Vector *vec;
    } func_decl;
} Node;

typedef struct CompoundStatement {
    int length;
    Node *ast[100];
} CompoundStatement;

extern Node *func_decl(Lexer *l);
extern CompoundStatement init_stmt();
extern void add_ast(CompoundStatement *p, Node *n);

// code.c
extern void emit_prologue(void);
extern void emit_epilogue(Node *n, int length, int count);
extern void emit_func_decl(Node *n);
extern void emit_func_ret(void);
extern void emit_code(Node *n);
extern void emit_lvalue_code(Vector *vec, Node *n);
extern void print_ast(Node *n);

#endif