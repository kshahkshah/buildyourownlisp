#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "repl.h"
#include "lib.h"

//
//
// LIFE CYCLE
//
//

lval* lval_pop(lval* expression, int index) {
  // store our target pointer value
  lval* value = expression->cell[index];

  // move the move memory over one
  // arguments are pointer to the destination, the data being copied, the number of bytes to copy
  memmove(&expression->cell[index], &expression->cell[index + 1],
    sizeof(lval*) * (expression->count - index - 1));

  // decrement the count
  expression->count--;

  // reallocate memory (drop the space for the final pointer)
  expression->cell = realloc(expression->cell, sizeof(lval*) * expression->count);

  return value;
}

lval* lval_take(lval* val, int index) {
  // pop the i'th value
  lval *target = lval_pop(val, index);
  // free up the lval itself
  lval_del(val);
  // return the target though
  return target;
}

void lval_del(lval* v) {
  switch (v->type) {
    case LVAL_FUN:
      // no extra work is required to delete built-in functions
      // but user space functions get nuked
      if (!v->builtin) {
        lenv_del(v->env);
        lval_del(v->formals);
        lval_del(v->body);
        break;
      }
    case LVAL_NUM: break;

    // we have to free the error message / symbol
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;

    // release the cells
    case LVAL_QEXPR:
    case LVAL_SEXPR:
      for (int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }
      // free the pointers
      free(v->cell);
      break;
  }

  free(v);
}

// in-place modifies list
lval* lval_add(lval* list, lval* incoming) {
  list->count++;
  list->cell = realloc(list->cell, sizeof(lval*) * list->count);
  list->cell[list->count - 1] = incoming;

  return list;
}

// in-place modifies list
lval* lval_unshift(lval* list, lval* incoming) {
  // make it bigger
  list->count++;
  list->cell = realloc(list->cell, sizeof(lval*) * list->count);

  // shift existing memory
  // arguments are pointer to the destination, the data being copied, the number of bytes to copy
  memmove(&list->cell[1], &list->cell[0],
    sizeof(lval*) * (list->count - 1));

  list->cell[0] = incoming;

  return list;
}

lval* lval_join(lval* x, lval* y) {

  /* For each cell in 'y' add it to 'x' */
  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  /* Delete the empty 'y' and return 'x' */
  lval_del(y);
  return x;
}

lval* lval_copy(lval* org) {
  lval* dup = malloc(sizeof(lval));
  dup->type = org->type;

  switch (dup->type) {
    case LVAL_FUN:
      if (org->builtin) {
        dup->builtin = org->builtin;
      } else {
        dup->env = lenv_copy(org->env);
        dup->formals = lval_copy(org->formals);
        dup->body = lval_copy(org->body);
      }
      break;
    case LVAL_NUM:
      dup->num = org->num;
      break;
    case LVAL_SYM:
      dup->sym = malloc(strlen(org->sym) + 1);
      strcpy(dup->sym, org->sym);
      break;
    case LVAL_ERR:
      dup->err = malloc(strlen(org->err) + 1);
      strcpy(dup->sym, org->sym);
      break;

    case LVAL_SEXPR:
    case LVAL_QEXPR:
      dup->count = org->count;
      dup->cell = malloc(sizeof(lval*) * dup->count);

      for(int i = 0; i < dup->count; i++) {
        dup->cell[i] = lval_copy(org->cell[i]);
      }
    break;
  }
  return dup;
}

//
//
// EVALUATION
//
//
lval* lval_call(lenv* env, lval* fn, lval* args) {
  // immediately return builtin functions, thats easy
  if (fn->builtin) {
    return fn->builtin(env, args);
  }

  // are we create a new expression or evaluating?
  int given = args->count;
  int total = fn->formals->count;

  while(args->count) {
    if (fn->formals->count == 0) {
      lval_del(args);
      return lval_err("function '%s' passed too many arguments, %i for %i",
               fn->sym, given, total);
    }

    lval* ref = lval_pop(fn->formals, 0);
    lval* val = lval_pop(args, 0);

    // place into the functions local environment
    lenv_put(fn->env, ref, val);

    // then cleanup
    lval_del(ref);
    lval_del(val);
  }

  // everything is bound at this point, clean this up
  lval_del(args);

  // now actually evaluate
  if(fn->formals->count == 0) {
    // we're going to create a new sexpr here,
    // and in place add a copy of the function body as it's first argument
    // and builtin_eval will then be able to evaluate that function body
    // within the context of the environment to which we've just added vars too
    lval* newexpr = lval_add(lval_sexpr(), lval_copy(fn->body));
    return builtin_eval(fn->env, newexpr);

  // or return a new expression
  } else {
    // return a copy of
    return lval_copy(fn);
  }
}

lval* lval_eval(lenv* env, lval* val) {
  if (val->type == LVAL_SYM) {
    lval* reference = lenv_get(env, val);
    lval_del(val);

    return reference;
  }

  if (val->type == LVAL_SEXPR) { return lval_eval_sexpr(env, val); }

  // or just return self
  return val;
}

lval* lval_eval_sexpr(lenv* env, lval* expr) {

  // evaluate children
  for (int i = 0; i < expr->count; i++) {
    // point to the evaluation results on this exprs cells
    expr->cell[i] = lval_eval(env, expr->cell[i]);

    // if we hit an error though, return immediately...
    if (expr->cell[i]->type == LVAL_ERR) { return lval_take(expr, i); };
  }

  // an empty expr case, return self
  if (expr->count == 0) { return expr; }

  // single expr, return the inner
  if (expr->count == 1) { return lval_take(expr, 0); }

  // validate syntax, the first element must as always be an function
  lval* fn = lval_pop(expr, 0);

  if (fn->type != LVAL_FUN) {
    lval* err = lval_err("incorrect type found when evaluating a symbolic expression, '%s' is not a function", fn->sym);
    lval_del(fn);
    lval_del(expr);
    return err;
  }

  // Actually evaluate
  lval* result = lval_call(env, fn, expr);

  // free the operand
  lval_del(fn);

  return result;
}
