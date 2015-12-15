#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include <math.h>

#include "mpc.h"

enum { LVAL_NUM, LVAL_ERR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM, LERR_WRONG_ARGS };

typedef struct {
  int type;
  long num;
  int err;
} lval;

lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num  = x;

  return v;
}

lval lval_err(int x) {
  lval e;
  e.type = LVAL_ERR;
  e.err = x;

  return e;
}

void lval_print(lval v) {
  switch (v.type) {
    case LVAL_NUM: printf("lispy> %li", v.num); break;
    case LVAL_ERR:
      if (v.err == LERR_DIV_ZERO) {
        printf("Error: Division By Zero!");
      }
      if (v.err == LERR_BAD_OP) {
        printf("Error: No Such Function!");
      }
      if (v.err == LERR_BAD_NUM) {
        printf("Error: Argument Too Large!");
      }
      if (v.err == LERR_WRONG_ARGS) {
        printf("Error: Wrong Number of Arguments!");
      }
    break;
  }
}

void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval eval_op(lval x, char* op, lval y) {

  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  // math
  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) {
    return y.num == 0
      ? lval_err(LERR_DIV_ZERO)
      : lval_num(x.num / y.num);
  }
  if (strcmp(op, "\%") == 0) {  return lval_num(x.num % y.num); }
  if (strcmp(op, "^") == 0)  {  return lval_num(powl(x.num, y.num)); }

  // comparison
  if (strcmp(op, "min") == 0) {
    if (x.num > y.num) {
      return y;
    } else {
      return x;
    }
  }
  if (strcmp(op, "max") == 0) {
    if (x.num > y.num) {
      return x;
    } else {
      return y;
    }
  }

  // we didn't catch the op, so it must be baddddd, throw an error
  return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* tree) {

  if (strstr(tree->tag, "number")) {
    errno = 0;

    // strtol takes pointer, a reference to its terminal null character (duh), and the base as a basis of interpretation of the string
    long x = strtol(tree->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  // the op is always the first argument
  char* op = tree->children[1]->contents;

  // the second child is the first argument
  lval x = eval(tree->children[2]);

  // the third child is the 'y' which we will accumulate in x
  int i = 3;

  // so this will process all remaining arguments which are expressions
  // just 'regex' wont match which is the 0 and 5th child...
  while (strstr(tree->children[i]->tag, "expr")) {
    // curry into x
    x = eval_op(x, op, eval(tree->children[i]));
    i++;
  }

  return x;
}

int main(int argc, char** argv) {

  /* Create Some Parsers */
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lispy    = mpc_new("lispy");

  /* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                                     \
      number   : /-?[0-9]+/ ;                                             \
      operator : '+' | '-' | '*' | '/' | '\%' | '^' | \"min\" | \"max\";      \
      expr     : <number> | '(' <operator> <expr>+ ')' ;                  \
      lispy    : /^/ <operator> <expr>+ /$/ ;                             \
    ",
    Number, Operator, Expr, Lispy);

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
      lval result = eval(r.output);
      lval_println(result);
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
  mpc_cleanup(4, Number, Operator, Expr, Lispy);

  return 0;
}
