#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include <math.h>

#include "mpc.h"
#include "repl.h"

lenv* lenv_new(void) {
  lenv* env = malloc(sizeof(lenv));
  // just like cell
  env->count = 0;
  env->syms = NULL;
  env->vals = NULL;

  return env;
}

void lenv_del(lenv* env) {
  // free the references in the syms, and the lvals they refer to
  for(int i = 0; i < env->count; i++) {
    free(env->syms[i]);
    lval_del(env->vals[i]);
  }

  // free pointers to the start of the reference array and values array
  free(env->syms);
  free(env->vals);

  // free pointer to the environment
  free(env);
}

lval* lenv_get(lenv* env, lval* key) {

  // since we're not using a hash of any sort iterate over the entire thing! wheeeeeeeeeeeeeeeeeeeeeeeeee...n
  for(int i = 0; i < env->count; i++) {
    if (strcmp(env->syms[i], key->sym) == 0) {
      // return a copy of the value
      return lval_copy(env->vals[i]);
    }
  }
  return lval_err("no such symbol");
}

void lenv_put(lenv* env, lval* key, lval* value) {

  // iterate over existing keys and replace
  for(int i = 0; i < env->count; i++) {
    if (strcmp(env->syms[i], key->sym) == 0) {
      // free old references
      lval_del(env->vals[i]);
      // update value and return
      env->vals[i] = lval_copy(value);
      return;
    }
  }

  // or insert

  // increment count and resize to it
  env->count++;
  env->vals = realloc(env->vals, sizeof(lval*) * env->count);
  env->syms = realloc(env->syms, sizeof(char*) * env->count);

  // copy the lispy value, store a pointer to it at the end of env's vals array
  env->vals[env->count-1] = lval_copy(value);
  // allocate space for the reference we were given
  env->syms[env->count-1] = malloc(strlen(key->sym) + 1);
  // store the actual bytes
  strcpy(env->syms[env->count-1], key->sym);
}

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

//
//
// lisp value generic operations on instances
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

void lval_del(lval* v) {
  switch (v->type) {
    // no extra work is required to delete this data type...
    case LVAL_FUN: break;
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

lval* lval_add(lval* list, lval* incoming) {
  list->count++;
  list->cell = realloc(list->cell, sizeof(lval*) * list->count);
  list->cell[list->count - 1] = incoming;

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
      dup->fun = org->fun;
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

// straight copied, just maps enum to a label
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

//
//
// PRINTING UTILITY FUNCTIONS
//
//

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

// straight copied
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

// straight copied
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

// straight copied and added too
// we're getting back copies of these lvals so free them
void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
  lval* k = lval_sym(name);
  lval* v = lval_fun(func);
  lenv_put(e, k, v);
  lval_del(k); lval_del(v);
}

void lenv_add_builtins(lenv* e) {
  /* List Functions */
  lenv_add_builtin(e, "def",  builtin_def);
  lenv_add_builtin(e, "min",  builtin_min);
  lenv_add_builtin(e, "max",  builtin_max);
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);

  /* Mathematical Functions */
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
  lenv_add_builtin(e, "\%", builtin_mod);
  lenv_add_builtin(e, "^", builtin_exp);
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

lval* lval_take(lval* val, int index) {
  // pop the i'th value
  lval *target = lval_pop(val, index);
  // free up the lval itself
  lval_del(val);
  // return the target though
  return target;
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
    lval_del(fn);
    lval_del(expr);
    return lval_err("error, not a function");
  }

  // Actually evaluate
  lval* result = fn->fun(env, expr);

  // free the operand
  lval_del(fn);

  return result;
}

lval* lval_read_num(mpc_ast_t* tree) {
  errno = 0;

  // strtol takes pointer, a reference to its terminal null character (duh), and the base as a basis of interpretation of the string
  long x = strtol(tree->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}


lval* lval_read(mpc_ast_t* tree) {
  if (strstr(tree->tag, "number")) { return lval_read_num(tree); }
  if (strstr(tree->tag, "symbol")) { return lval_sym(tree->contents); }

  // empty lists are valid
  lval* x = NULL;
  // > is an mpc tag, meaning this is children, so start an sexpr
  if (strcmp(tree->tag, ">") == 0) { x = lval_sexpr(); }

  // this is already tagged as an qexpr
  if (strstr(tree->tag, "qexpr")) { x = lval_qexpr(); }

  // this is already tagged as an sexpr
  if (strstr(tree->tag, "sexpr")) { x = lval_sexpr(); }

  for (int i = 0; i < tree->children_num; i++) {
    // skip grammar
    if (strcmp(tree->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(tree->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(tree->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(tree->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(tree->children[i]->tag,  "regex") == 0) { continue; }

    // data, add it to the sexpr
    x = lval_add(x, lval_read(tree->children[i]));
  }

  return x;
}

int main(int argc, char** argv) {

  /* Create Some Parsers */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr  = mpc_new("sexpr");
  mpc_parser_t* Qexpr  = mpc_new("qexpr");
  mpc_parser_t* Expr   = mpc_new("expr");
  mpc_parser_t* Lispy  = mpc_new("lispy");

  /* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                                     \
      number   : /-?[0-9]+/ ;                                             \
      symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&^\%]+/ ;                    \
      qexpr    : '{' <expr>* '}' ;                                        \
      sexpr    : '(' <expr>* ')' ;                                        \
      expr     : <number> | <symbol> | <sexpr> | <qexpr>;                 \
      lispy    : /^/ <expr>* /$/ ;                                        \
    ",
    Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

  /* Print Version and Exit Information */
  puts("Lispy Version 0.0.0.0.1");
  puts("Press Ctrl+c to Exit\n");

  // create environment and add functions
  lenv* env = lenv_new();
  lenv_add_builtins(env);

  /* In a never ending loop */
  while (1) {

    /* Output our prompt and get input */
    char* input = readline("lispy> ");
    mpc_result_t r;

    /* Add input to history */
    add_history(input);

    /* Attempt to Parse the user Input */
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      lval* x = lval_eval(env, lval_read(r.output));
      lval_println(x);
      lval_del(x);
      mpc_ast_delete(r.output);
    } else {
      /* Otherwise Print the Error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    /* Free retrieved input */
    free(input);

  }

  lenv_del(env);

  /* Undefine and Delete our Parsers */
  mpc_cleanup(4, Number, Symbol, Qexpr, Sexpr, Expr, Lispy);

  return 0;
}
