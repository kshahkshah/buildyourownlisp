#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "repl.h"

void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_FUN: printf("<function>"); break;
    case LVAL_NUM: printf("%li", v->num); break;
    case LVAL_SYM: printf("%s", v->sym); break;
    case LVAL_ERR: printf("Error: %s", v->err); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
  }
}

void lval_println(lval* v) {
  lval_print(v);
  putchar('\n');
}

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);

    // give some breathing room to all except the last element as printing
    if (i != (v->count - 1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

// we're just mapping enums to a label
char* lval_human_name(int t) {
  switch(t) {
    case LVAL_FUN: return "Function";
    case LVAL_NUM: return "Number";
    case LVAL_ERR: return "Error";
    case LVAL_SYM: return "Symbol";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    default: return "Unknown";
  }
}
