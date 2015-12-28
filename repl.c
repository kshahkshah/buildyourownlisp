#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "mpc.h"
#include "repl.h"

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
      symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>|!&^\%]+/ ;                   \
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

  // add some shortcuts too;
  lenv_def(env, lval_sym("true"), lval_bool(1));
  lenv_def(env, lval_sym("false"), lval_bool(0));

  int no_exit_signal = 1;

  /* In a never ending loop */
  while (no_exit_signal) {

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

    lval* quit_signal = lenv_get(env, lval_sym("__quit__"));

    if (quit_signal->type == LVAL_SIG) {
      // we have an exit signal...
      no_exit_signal = 0;
    }
  }

  lenv_del(env);

  /* Undefine and Delete our Parsers */
  mpc_cleanup(4, Number, Symbol, Qexpr, Sexpr, Expr, Lispy);

  return 0;
}
