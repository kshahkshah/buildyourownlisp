#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "repl.h"
#include "lib.h"

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

//
// removes the first item of a quoted expression
//
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

//
// changes the type of an lval to a quoted expression
//
lval* builtin_quote(lenv* env, lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_length(lenv* env, lval* a) {
  lval* x = lval_num(a->count);
  return x;
}

lval* builtin_cons(lenv* env, lval* args) {
  LASSERT(args, args->cell[0]->type == LVAL_QEXPR,
    "Error! Functon 'cons' must be passed a quoted expression\n"
    "But was passed a %s", lval_human_name(args->cell[0]->type));

  lval* list = lval_copy(args->cell[0]);

  while (args->count > 1) {
    list = lval_unshift(list, lval_copy(lval_pop(args, 1)));
  }

  lval_del(args);

  return list;
}

// switch from QEXPR -> SEXPR and evaluate a child
lval* builtin_eval(lenv* env, lval* a) {
  LASSERT_ARITY("eval", a, 1);
  LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

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

lval* builtin_def(lenv* env, lval* a) {
  return builtin_var(env, a, "def");
}
lval* builtin_put(lenv* env, lval* a) {
  return builtin_var(env, a, "=");
}

// add variables to the environment
lval* builtin_var(lenv* env, lval* args, char *op) {
  // we can only process quoted expressions unfortunately atm
  // because anything else will be evaluated
  LASSERT(args, args->cell[0]->type == LVAL_QEXPR,
    "'%s' passed an unquoted expression!", op);

  // grab the references (var names)
  lval* refs = args->cell[0];

  // make sure they are indeed symbols
  for(int i = 0; i < refs->count; i++) {
    LASSERT(args, refs->cell[i]->type == LVAL_SYM,
      "'%s' passed invalid symbols!", op);
  }

  // also make sure we were given the same number of values
  LASSERT(args, refs->count == args->count-1,
    "'%s' passed incorrect number of symbols", op);

  // iterate over and assign
  for (int i = 0; i < refs->count; i++) {
    // global
    if (strcmp(op, "def") == 0) {
      lenv_def(env, refs->cell[i], args->cell[i+1]);
    }

    // local
    if (strcmp(op, "=") == 0) {
      lenv_put(env, refs->cell[i], args->cell[i+1]);
    }
  }

  lval_del(args);
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

lval* builtin_locals(lenv* env, lval* a) {
  // we might as well use the empty qexpr which was passed to us...

  // create symbols and add them to a
  for(int i = 0; i < env->count; i++) {
    if (lenv_get(env, lval_sym(env->syms[i]))->type != LVAL_FUN) {
      a = lval_add(a, lval_sym(env->syms[i]));
    }
  }

  return a;
}

lval* builtin_functions(lenv* env, lval* a) {
  // we might as well use the empty qexpr which was passed to us...

  // create symbols and add them to a
  for(int i = 0; i < env->count; i++) {
    if (lenv_get(env, lval_sym(env->syms[i]))->type == LVAL_FUN) {
      a = lval_add(a, lval_sym(env->syms[i]));
    }
  }

  return a;
}

lval* builtin_lambda(lenv* env, lval* a) {
  // there are two arguments
  LASSERT_ARITY("lambda", a, 2);
  // whose type are quoted expressions
  LASSERT_TYPE("lambda", a, 0, LVAL_QEXPR);
  LASSERT_TYPE("lambda", a, 1, LVAL_QEXPR);

  // we should be getting passed symbols
  // which are arguments to the function we are defining
  // so check that fact
  for(int i = 0; i < a->cell[0]->count; i++) {
    // check the type of every child (used defined arg reference)
    // and give an unhelpful error message if violated
    LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
      "function definitions only take symbols as arguments, but the argument at index %i is a %s",
      i, lval_human_name(a->cell[0]->cell[i]->type));
  }

  // gets copies of the required formal arguments and body
  lval *formals = lval_pop(a, 0);
  lval *body = lval_pop(a, 0);
  // kill what was passed in
  lval_del(a);

  return lval_lambda(formals, body);
}

// returns 0 or 1 if symbol defined or not
lval* builtin_exists(lenv* env, lval* a) {
  // we can only process quoted expressions unfortunately atm
  // because anything else will be evaluated
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "def passed an unquoted expression!");

  // grab the references (var names)
  lval* ref = a->cell[0];

  // make sure they are indeed symbols
  LASSERT(a, ref->cell[0]->type == LVAL_SYM,
    "def passed invalid symbols!");

  lval* x = lenv_get(env, ref->cell[0]);
  int e = (x->type == LVAL_ERR) ? 0 : 1;
  lval_del(x);

  return lval_num(e);
}

lval* builtin_exit(lenv* env, lval* a) {
  lenv_put(env, lval_sym("__quit__"), lval_sig(0));
  lval_del(a);

  return lval_sym("Exiting!");
}
