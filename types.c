#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "repl.h"

//
//
// lisp value instance types
//
//

// returns a pointer to a number
// takes the value of the number (long)
lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;

  return v;
}

// returns a pointer to an error lval
// takes a message
lval* lval_err(char* message, ...) {
  lval* e = malloc(sizeof(lval));
  e->type = LVAL_ERR;

  va_list va;
  va_start(va, message);

  // allocate a finite space...
  e->err = malloc(512);

  vsnprintf(e->err, 511, message, va);

  va_end(va);

  return e;
}

// returns a pointer to an symbol lval
// takes the value of the symbol
lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym  = malloc(strlen(s) + 1);
  strcpy(v->sym, s);

  return v;
}

// return a pointer to a quoted expression
lval* lval_qexpr(void) {
  lval* q = malloc(sizeof(lval));
  q->type = LVAL_QEXPR;

  q->count = 0;
  q->cell = NULL;

  return q;
}

// return a pointer to an s-expression
lval* lval_sexpr(void) {
  lval* s = malloc(sizeof(lval));
  s->type = LVAL_SEXPR;

  // initialize at zero since we're taking no arguments...
  s->count = 0;
  s->cell = NULL;

  return s;
}

lval* lval_fun(lbuiltin fn) {
  lval* f = malloc(sizeof(lval));
  f->type = LVAL_FUN;
  f->fun = fn;
  return f;
}
