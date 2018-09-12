#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "./knicc.h"

int main(int argc, char **argv) {
    l = init_lexer();
    global_map = init_map();
    string_literal_vec = init_vector();
    fgets(l->src, 1000, stdin);
    while (1) {
        Token t = lex();
        store_token(t);
        // debug_token(t);
        if (t.type == _EOF) break;
    }
    Vector *nodes = init_vector();
    while (peek_token(&l).type != _EOF) {
        vec_push(nodes, external_declaration(&l));
    }
    emit_toplevel(nodes);
    return 0;
}
