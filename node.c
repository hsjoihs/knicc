#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "knicc.h"

Node* assign(Lexer *l);
Node* expr(Lexer *l);
Node* term(Lexer *l);
Node* factor(Lexer *l);

CompoundStatement init_stmt() {
    CompoundStatement stmt;
    stmt.length = 0;
    return stmt;
}

void add_ast(CompoundStatement *stmt, Node *n) {
    stmt->ast[stmt->length] = n;
    stmt->length += 1;
}

Node *make_ast_int(int val) {
    Node *n = malloc(sizeof(Node));
    n->type = INT;
    n->ival = val;
    return n;
}

Node *make_ast_op(int type, Node *left, Node *right) {
    Node *n = malloc(sizeof(Node));
    n->type = type;
    n->left = left;
    n->right = right;
    return n;
}

Node *make_ast_ident(char *literal) {
    Node *n = malloc(sizeof(Node));
    n->literal = malloc(sizeof(char) * strlen(literal));
    strcpy(n->literal, literal);
    n->type = IDENT;
    return n;
}

Node *make_ast_func_call(char *func_name, int argc, int *argv) {
    Node *n = malloc(sizeof(Node));
    n->type = FUNC_CALL;
    n->func_call.func_name = malloc(sizeof(char) * strlen(func_name));
    strcpy(n->func_call.func_name, func_name);
    n->func_call.argc = argc;
    n->func_call.argv = malloc(sizeof(int) * argc);
    if (n->func_call.argv == NULL) perror("malloc err");
    n->func_call.argv = argv;
    return n;
}

Node *make_ast_func_decl(char *func_name) {
    Node *n = malloc(sizeof(Node));
    n->type = FUNC_DECL;
    n->func_decl.func_name = malloc(sizeof(char) * strlen(func_name));
    strcpy(n->func_decl.func_name, func_name);
    n->func_decl.stmt = malloc(sizeof(CompoundStatement));
    n->func_decl.vec = init_vector();
    return n;
}

Node *func_decl(Lexer *l) {
    Token t = get_token(l);
    assert(t.type == IDENT);
    char *func_name = t.literal;
    assert(get_token(l).type == LParen);
    assert(get_token(l).type == RParen);
    assert(get_token(l).type == LBrace);
    Node *func_ast = make_ast_func_decl(func_name);
    while (peek_token(l).type != RBrace) {
        Node *n = assign(l);
        add_ast(n->func_decl.stmt, n);
        if (is_binop(n->type) && n->left != NULL && n->left->type == IDENT) {
            vec_push(func_ast->func_decl.vec, new_kv(n->left->literal, func_ast->func_decl.vec->length * -4));
        }
        assert(get_token(l).type == SEMICOLON);
    }
    assert(get_token(l).type == RBrace);
    return func_ast;
}

Node *assign(Lexer *l) {
    Node *left = expr(l);
    Token t = peek_token(l);
    while (t.type == ASSIGN) {
        Token op = get_token(l);
        Node *right = expr(l);
        left = make_ast_op(op.type, left, right);
        t = peek_token(l);
    }
    return left;
}

Node *expr(Lexer *l) {
    Node *left = term(l);
    Token t = peek_token(l);
    while (t.type == ADD || t.type == SUB) {
        Token op = get_token(l);
        Node *right = term(l);
        left = make_ast_op(op.type, left, right);
        t = peek_token(l);
    }
    return left;
}

Node *term(Lexer *l) {
    Node *left = factor(l);
    Token t = peek_token(l);
    while (t.type == MULTI) {
        Token op = get_token(l);
        Node *right = term(l);
        left = make_ast_op(op.type, left, right);
        t = peek_token(l);
    }
    return left;
}

Node *factor(Lexer *l) {
    Token t = get_token(l);
    if (t.type == INT) return make_ast_int(atoi(t.literal));
    else if (t.type == IDENT) {
        if (peek_token(l).type == LParen) {
            get_token(l);
            int argv[6];
            int argc = 0;
            while (1) {
                if (peek_token(l).type == RParen) break;
                if (peek_token(l).type == INT) {
                    argv[argc] = atoi(get_token(l).literal);
                    argc += 1;
                }
                if (peek_token(l).type == COMMA) get_token(l);
            }
            assert(get_token(l).type == RParen);
            if (peek_token(l).type == SEMICOLON) return make_ast_func_call(t.literal, argc, argv);
            assert(get_token(l).type == LBrace);

        }
        return make_ast_ident(t.literal);
    }
    debug_token(t);
    assert(t.type == LParen);
    Node *left = expr(l);
    assert(get_token(l).type == RParen);
    return left;
}