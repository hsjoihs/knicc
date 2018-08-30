#include <stdio.h>
#include <stdbool.h>

#include "knicc.h"

Map *map;
Map *global_map;

int func_offset;

const char *regs[6] = {
    "rdi",
    "rsi",
    "rdx",
    "rcx",
    "r8",
    "r9"
};

void emit_global_var(void) {
    for (int i = 0; i < global_map->vec->length; i++) {
        char *key = ((KeyValue *)(vec_get(global_map->vec, i)))->key;
        printf("%s:\n", key);
        printf("  .zero 4\n");
    }
}

void emit_toplevel(Vector *n) {
    printf(".data\n");
    emit_global_var();
    printf(".text\n");
    printf(".global main\n");
    for (int i = 0; i < n->length; i++) {
        Node *ast = vec_get(n, i);
        if (ast->type == GLOBAL_DECL) continue;
        else emit_func_decl(ast);
    }
}

void emit_mov_args(int argc) {
    for (int i = 0; i < argc; i++) {
        printf("  movq  %%%s, -%d(%%rbp)\n", regs[i], (i+1) * 8);
    }
}

void emit_func_decl(Node *n) {
    printf("%s:\n", n->func_decl.func_name);
    printf("  pushq %%rbp\n");
    printf("  movq %%rsp, %%rbp\n");
    map = n->func_decl.map;
    func_offset = n->offset;
    printf("  sub $%d, %%rsp\n", func_offset);
    emit_mov_args(n->func_decl.argc);
    for (int i = 0; i < n->compound_stmt.block_item_list->length; i++) {
        Node *ast = vec_get(n->compound_stmt.block_item_list, i);
        emit_expr(ast);
    }
    emit_func_ret(n);
}

void emit_func_ret(Node *n) {
    printf("\n  pop %%rax\n");
    printf("  add $%d, %%rsp\n", func_offset);
    printf("  mov %%rbp, %%rsp\n");
    printf("  pop %%rbp\n");
    printf("  ret\n");
}

void emit_args(Node *n) {
    for (int i = 0; i < n->func_call.argc; i++) {
        if (n->func_call.argv[i]->type == INT) {
            printf("  mov  $%d,  %%%s\n", n->func_call.argv[i]->ival, regs[i]);
        } else {
            emit_expr(n->func_call.argv[i]);
            printf("  pop %%rax\n");
            printf("  mov  %%rax,  %%%s\n", regs[i]);
        }
    }
}

void gen_operands(void) {
    printf("  popq %%rax\n");
    printf("  popq %%rdx\n");
}

void emit_code(Node *n) {
    Vector *stmts;
    Var *v;
    KeyValue *kv;
    if (n == NULL) return;
    switch(n->type) {
        case INT:
            printf("  push $%d\n", n->ival);
            break;
        case ADD:
            gen_operands();
            printf("  add %%rdx, %%rax\n");
            printf("  push %%rax\n");
            break;
        case SUB:
            gen_operands();
            printf("  sub %%rax, %%rdx\n");
            printf("  pushq %%rdx\n");
            break;
        case MULTI:
            gen_operands();
            printf("  imul %%rdx, %%rax\n");
            printf("  push %%rax\n");
            break;
        case ASSIGN:
            emit_lvalue_code(n->left);
            emit_expr(n->right); // expr
            // 代入を実行
            printf("  pop %%rbx\n");
            printf("  pop %%rax\n");
            printf("  mov %%rbx, (%%rax)\n");
            printf("  push %%rbx\n");
            break;
        case LESS:
            printf("  popq %%rdx\n");
            printf("  popq %%rax\n");
            printf("  cmpq %%rdx, %%rax\n");
            printf("  setl %%al\n");
            printf("  movzbl %%al, %%eax\n");
            printf("  push %%rax\n");
            break;
        case IDENTIFIER:
            kv = ((KeyValue *)(find_by_key(map, n->literal)));
            if (kv != NULL) {
                v = kv->value;
                if (v->type == TYPE_ARRAY) printf("  leaq %d(%%rbp), %%rax\n", v->offset);
                else if (v->type == TYPE_INT || v->type == TYPE_INT_PTR) printf("  mov %d(%%rbp), %%rax\n", v->offset);
                else if (v->type == TYPE_CHAR) printf("  movzbl %d(%%rbp), %%rax\n", v->offset);
            } else printf("  mov %s(%%rip), %%rax\n", n->literal);
            printf("  pushq %%rax\n");
            break;
        case FUNC_CALL:
            emit_args(n);
            printf("  call %s\n", n->func_call.func_name);
            printf("  push %%rax\n");
            break;
        case COMPOUND_STMT:
            stmts = n->compound_stmt.block_item_list;
            for (int i = 0; i < stmts->length; i++) {
                Node *ast = vec_get(stmts, i);
                printf("\n// %d line\n", i+1);
                emit_expr(ast);
            }
            break;
        case IF_STMT:
            emit_expr(n->if_stmt.expression);
            printf("  pop %%rax\n");
            printf("  cmpq $0, %%rax\n");
            printf("  je .Lend\n");
            emit_expr(n->if_stmt.true_stmt);
            printf(".Lend:\n");
            break;
        case WHILE:
            printf(".Lbegin:\n");
            emit_expr(n->while_stmt.expression);
            printf("  pop %%rax\n");
            printf("  cmpq $0, %%rax\n");
            printf("  je .Lend\n");
            emit_expr(n->while_stmt.stmt); 
            printf("  jmp .Lbegin\n");
            printf(".Lend:\n");
            break;
        case FOR:
            emit_expr(n->for_stmt.init_expr);
            printf(".Lbegin:\n");
            emit_expr(n->for_stmt.cond_expr);
            printf("  pop %%rax\n");
            printf("  cmpq $0, %%rax\n");
            printf("  je .Lend\n");
            emit_expr(n->for_stmt.stmt); 
            emit_expr(n->for_stmt.loop_expr);
            printf("  jmp .Lbegin\n");
            printf(".Lend:\n");
            break;
        case REF:
            v = get_first_var(map, n);
            printf("  leaq %d(%%rbp),  %%rax\n", v->offset);
            printf("  push %%rax  \n");
            break;
        case DEREF:
            v = get_first_var(map, n);
            emit_expr(n->left); // スタックのトップに p+12 とかのアドレスが乗ってる
            // emit_expr(n->right); segfault
            // printf("  movq %d(%%rbp), %%rax\n", var->offset);
            printf("  pop %%rax\n");
            printf("  push (%%rax)  \n");
            break;
        case RETURN:
            emit_expr(n->ret_stmt.expr);
            printf("  pop %%rax\n");
            printf("  add $%d, %%rsp\n",func_offset);
            printf("  leave\n");
            printf("  ret\n");
            break;
        default:
            printf("type of n->type: %d\n", n->type);
            debug_token(new_token("", n->type));
            return;
    }
}

void emit_expr(Node *n) {
    if (n == NULL) return;
    if (is_binop(n->type)) {
        if (n->type == ADD) {
            if (n->left->type == IDENTIFIER) {
                emit_expr(n->left);
                emit_expr(n->right);
                KeyValue *kv = find_by_key(map, n->left->literal);
                if (kv != NULL) {
                    TrueType ty = ((Var *)(kv->value))->type;
                    printf("  pushq $%d\n", add_sub_ptr(ty));
                    gen_operands();
                    printf("  imul %%rdx, %%rax\n");
                    printf("  push %%rax\n");
                }
                emit_code(n);
                return;
            }
        }
        emit_expr(n->left);
        emit_expr(n->right);
    }
    // print_ast(n);
    emit_code(n);
}

void emit_lvalue_code(Node *n) {
    if (n->type == DEREF) {
        emit_expr(n->left);
    } else if (n->type == IDENTIFIER) {
        KeyValue *kv = ((KeyValue *)(find_by_key(map, n->literal)));
        if (kv != NULL){
            Var *v = ((Var *)(kv->value));
            printf("  leaq %d(%%rbp), %%rax\n", v->offset);
        }
        else printf("  leaq %s(%%rip), %%rax\n", n->literal);
    }
    printf("  pushq %%rax\n");
}

void print_ast(Node *node) {
    switch (node->type) {
        case ASSIGN:
            printf("(define ");
            print_ast(node->left);
            print_ast(node->right);
            printf(")");
            break;
        case IDENTIFIER:
            printf("%s ", node->literal);
            break;
        case ADD:
            printf("(+ ");
            goto print_op;
        case SUB:
            printf("(- ");
            goto print_op;
        case MULTI:
            printf("(* ");
            goto print_op;
        case LESS:
            printf("(< ");
            goto print_op;
        case MORE:
            printf("(> ");
            goto print_op;
            print_op:
            print_ast(node->left);
            printf(" ");
            print_ast(node->right);
            printf(")");
            break;
        case INT:
            printf("%d", node->ival);
            break;
        case FUNC_CALL:
            printf("(");
            printf("%s", node->func_call.func_name);
            for (int i = 0; i < node->func_call.argc; i++) {
                printf(" %d", node->func_call.argv[i]->ival);
            }
            printf(")\n");
            break;
        case IF_STMT:
            printf("(if ");
            print_ast(node->if_stmt.expression);
            print_ast(node->if_stmt.true_stmt);
            printf(")\n");
            break;
        default:
            debug_token(new_token("err", node->type));
            perror("should not reach here");
    }
}
