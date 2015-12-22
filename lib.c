#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "repl.h"

// straight copied then modified
lval* builtin_head(lenv* env, lval* a) {
  LASSERT(a, a->count == 1,
    "Function 'head' passed too many arguments!"
    "Received %i, Expected 1", a->count);

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'head' passed incorrect type!"
    "Received %s, Expected %s",
    lval_human_name(a->cell[0]->type),
    lval_human_name(LVAL_QEXPR));

  LASSERT(a, a->cell[0]->count != 0,
    "Function 'head' passed {}!");

  lval* v = lval_take(a, 0);

  while (v->count > 1) {
    lval_del(lval_pop(v, 1));
  }

  return v;
}

// straight copied then modified
lval* builtin_join(lenv* env, lval* a) {

  for (int i = 0; i < a->count; i++) {
    LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
      "Function 'join' passed incorrect type.");
  }

  lval* x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

// straight copied
lval* builtin_tail(lenv* env, lval* a) {
  LASSERT(a, a->count == 1,
    "Function 'tail' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'tail' passed incorrect type!");
  LASSERT(a, a->cell[0]->count != 0,
    "Function 'tail' passed {}!");

  lval* v = lval_take(a, 0);
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_list(lenv* env, lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

// switch from QEXPR -> SEXPR and evaluate a child
lval* builtin_eval(lenv* env, lval* a) {
  LASSERT(a, a->count == 1,
    "Function 'eval' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'eval' passed incorrect type!");

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;

  return lval_eval(env, x);
}

lval* builtin_min(lenv* env, lval* a) {

  lval* x = lval_pop(a, 0);

  while(a->count > 0) {
    lval* y = lval_pop(a, 0);

    if (x->num > y->num) {
      x->num = y->num;
    }

    lval_del(y);
  }

  lval_del(a);
  return x;
}

lval* builtin_max(lenv* env, lval* a) {
  lval* x = lval_pop(a, 0);

  while(a->count > 0) {
    lval* y = lval_pop(a, 0);

    if (y->num > x->num) {
      x->num = y->num;
    }

    lval_del(y);
  }

  lval_del(a);
  return x;
}

// add variables to the environment
lval* builtin_def(lenv* env, lval* refs_and_vals) {
  // we can only process quoted expressions unfortunately atm
  // because anything else will be evaluated
  LASSERT(refs_and_vals, refs_and_vals->cell[0]->type == LVAL_QEXPR,
    "Error: def passed an unquoted expression!");

  // grab the references (var names)
  lval* refs = refs_and_vals->cell[0];

  // make sure they are indeed symbols
  for(int i = 0; i < refs->count; i++) {
    LASSERT(refs_and_vals, refs->cell[i]->type == LVAL_SYM,
      "Error: def passed invalid symbols!");
  }

  // also make sure we were given the same number of values
  LASSERT(refs_and_vals, refs->count == refs_and_vals->count-1,
    "Error: def passed incorrect number of symbols");

  // iterate over and assign
  for (int i = 0; i < refs->count; i++) {
    lenv_put(env, refs->cell[i], refs_and_vals->cell[i+1]);
  }

  lval_del(refs_and_vals);
  return lval_sexpr();
}

lval* builtin_op(lenv* e, lval* a, char* op) {

  lval* x = lval_pop(a, 0);

  // check for single argument and negation operator,
  // this is really because we have an overloaded symbol, right?
  if ((strcmp(op, "-") == 0) && (a->count == 0)) {
    // return in place
    x->num = -x->num;
  }

  while(a->count > 0) {

    lval* y = lval_pop(a, 0);

    // math
    if (strcmp(op, "+") == 0) { x->num = (x->num + y->num); }
    if (strcmp(op, "-") == 0) { x->num = (x->num - y->num); }
    if (strcmp(op, "*") == 0) { x->num = (x->num * y->num); }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        // free these immediately, we're stopping execution
        lval_del(x);
        lval_del(y);

        // stamp the reference
        x = lval_err("Division by Zero!");
        break;
      }

      // okay we can divide...
      x->num = x->num / y->num;
    }
    if (strcmp(op, "\%") == 0) {  x->num = (x->num % y->num); }
    if (strcmp(op, "^") == 0)  {  x->num = (powl(x->num, y->num)); }

    // for all these ops we need to discard the argument as
    // we've applied it already and reduced into x
    lval_del(y);

  }

  // the expression passed in has been reduced to just x, so we can return that, and free a;
  lval_del(a);

  return x;
}

lval* builtin_add(lenv* e, lval* a) {
  return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a) {
  return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a) {
  return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a) {
  return builtin_op(e, a, "/");
}

lval* builtin_mod(lenv* e, lval* a) {
  return builtin_op(e, a, "\%");
}

lval* builtin_exp(lenv* e, lval* a) {
  return builtin_op(e, a, "^");
}
