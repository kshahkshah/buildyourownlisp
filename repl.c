#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include <math.h>

#include "mpc.h"

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

typedef lval*(*lbuiltin)(lenv*, lval*);

typedef struct lval {
  int type;
  long num;

  // we need a place to record string data...
  char *err;
  char *sym;
  lbuiltin fun;

  // count is the list length of an s-expression
  int count;

  // cell points to other list values' pointers
  struct lval** cell;
};

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
lval* lval_err(char* message) {
  lval* e = malloc(sizeof(lval));
  e->type = LVAL_ERR;
  e->err  = malloc(strlen(message) + 1);
  strcpy(e->err, message);

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

lval* lval_fun(lbuiltin func) {

}

#define LASSERT(args, cond, err) if (!(cond)) { lval_del(args); return lval_err(err); }

lval* lval_take(lval* val, int index);
lval* lval_eval(lval* val);
lval* lval_eval_sexpr(lval* expression);

lval* builtin_op(lval* a, char* op);
lval* builtin_max(lval* a);
lval* builtin_min(lval* a);
lval* builtin_eval(lval* a);
lval* builtin_list(lval* a);
lval* builtin_join(lval* a);
lval* builtin_tail(lval* a);
lval* builtin_head(lval* a);

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

//
//
// PRINTING UTILITY FUNCTIONS
//
//
void lval_print(lval* v);
void lval_println(lval* v);
void lval_expr_print(lval* v, char open, char close);

void lval_print(lval* v) {
  switch (v->type) {
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
lval* builtin_head(lval* a) {
  LASSERT(a, a->count == 1,
    "Function 'head' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'head' passed incorrect type!");
  LASSERT(a, a->cell[0]->count != 0,
    "Function 'head' passed {}!");

  lval* v = lval_take(a, 0);  

  while (v->count > 1) { 
    lval_del(lval_pop(v, 1));
  }

  return v;
}

// straight copied
lval* builtin_join(lval* a) {

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
lval* builtin_tail(lval* a) {
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

lval* builtin_list(lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

// switch from QEXPR -> SEXPR and evaluate a child
lval* builtin_eval(lval* a) {
  LASSERT(a, a->count == 1,
    "Function 'eval' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'eval' passed incorrect type!");

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(x);
}

lval* builtin_min(lval* a) {

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

lval* builtin_max(lval* a) {
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

lval* builtin(lval* a, char* func) {

  // // check to see if we can actually evaluate yet...
  // for (int i = 0; i < a->count; i++) {
  //   if (a->cell[i]->type != LVAL_NUM) {
  //     lval_del(a);
  //     // i suppose references can be caught here...

  //     // and if there are references 
  //     // which can not yet be resolved
  //     // return a quoted expression

  //     // instead of this:
  //     return lval_err("Not a Number!");
  //     // which should only be returned if we expected a number based off of the function call

  //   }
  // }

  if (strcmp("min", func) == 0)  { return builtin_min(a); }
  if (strcmp("max", func) == 0)  { return builtin_max(a); }
  if (strcmp("list", func) == 0) { return builtin_list(a); }
  if (strcmp("head", func) == 0) { return builtin_head(a); }
  if (strcmp("tail", func) == 0) { return builtin_tail(a); }
  if (strcmp("join", func) == 0) { return builtin_join(a); }
  if (strcmp("eval", func) == 0) { return builtin_eval(a); }
  if (strstr("+-/*^\%", func)) { return builtin_op(a, func); }

  // if not caught free and return!
  lval_del(a);
  return lval_err("No Such Function!");

}

lval* builtin_op(lval* a, char* op) {

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


lval* lval_eval(lval* val) {
  if (val->type == LVAL_SEXPR) { return lval_eval_sexpr(val); }

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

lval* lval_eval_sexpr(lval* expression) {

  // evaluate children
  for (int i = 0; i < expression->count; i++) {
    // point to the evaluation results on this expressions cells
    expression->cell[i] = lval_eval(expression->cell[i]);

    // if we hit an error though, return immediately...
    if (expression->cell[i]->type == LVAL_ERR) { return lval_take(expression, i); };
  }

  // an empty expression case, return self
  if (expression->count == 0) { return expression; }

  // single expression, return the inner
  if (expression->count == 1) { return lval_take(expression, 0); }

  // validate syntax, the first element must as always be an operator/function (symbol)
  lval* symbol = lval_pop(expression, 0);

  if (symbol->type != LVAL_SYM) {
    lval_del(symbol);
    lval_del(expression);
    return lval_err("Invalid Expression, first argument is an invalid symbol");
  }

  // Actually evaluate
  lval* evaluated = builtin(expression, symbol->sym);

  // free the operand
  lval_del(symbol);

  return evaluated;
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

  /* In a never ending loop */
  while (1) {

    /* Output our prompt and get input */
    char* input = readline("lispy> ");
    mpc_result_t r;

    /* Add input to history */
    add_history(input);

    /* Attempt to Parse the user Input */
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      lval* x = lval_eval(lval_read(r.output));
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

  /* Undefine and Delete our Parsers */
  mpc_cleanup(4, Number, Symbol, Qexpr, Sexpr, Expr, Lispy);

  return 0;
}
