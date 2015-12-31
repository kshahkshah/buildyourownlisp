#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "mpc.h"
#include "lispy.h"
#include "lib.h"

lval* lval_read_num(mpc_ast_t* tree) {
  errno = 0;

  // strtol takes pointer, a reference to its terminal null character (duh), and the base as a basis of interpretation of the string
  long x = strtol(tree->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval* lval_read_str(mpc_ast_t* tree) {
  // replace the " with a null terminator
  tree->contents[strlen(tree->contents)-1] = '\0';

  // copying this from the book, it's mpc stuff I'm not touching yet

  /* Copy the string missing out the first quote character */
  char* unescaped = malloc(strlen(tree->contents+1)+1);
  strcpy(unescaped, tree->contents+1);
  /* Pass through the unescape function */
  unescaped = mpcf_unescape(unescaped);
  /* Construct a new lval using the string */
  lval* str = lval_str(unescaped);
  /* Free the string and return */
  free(unescaped);
  return str;
}

lval* lval_read(mpc_ast_t* tree) {
  if (strstr(tree->tag, "number")) { return lval_read_num(tree); }
  if (strstr(tree->tag, "symbol")) { return lval_sym(tree->contents); }
  if (strstr(tree->tag, "string")) { return lval_read_str(tree); }

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
    if (strstr(tree->children[i]->tag, "comment")) { continue; }

    // data, add it to the sexpr
    x = lval_add(x, lval_read(tree->children[i]));
  }

  return x;
}

int main(int argc, char** argv) {

  /* Print Version and Exit Information */
  puts("Lispy Version 0.0.0.0.1");
  puts("Press Ctrl+c to Exit\n");

  // create empty parsers
  mpc_parser_t* Number  = mpc_new("number");
  mpc_parser_t* Symbol  = mpc_new("symbol");
  mpc_parser_t* String  = mpc_new("string");
  mpc_parser_t* Comment = mpc_new("comment");
  mpc_parser_t* Qexpr   = mpc_new("qexpr");
  mpc_parser_t* Sexpr   = mpc_new("sexpr");
  mpc_parser_t* Expr    = mpc_new("expr");
  mpc_parser_t* Lispy   = mpc_new("lispy");

  // fill the parsers with the lang
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                      \
      number   : /-?[0-9]+/ ;                              \
      symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>|!&^\%]+/ ;    \
      string   : /\"(\\\\.|[^\"])*\"/ ;                    \
      comment  : /;[^\\r\\n]*/ ;                           \
      qexpr    : '{' <expr>* '}' ;                         \
      sexpr    : '(' <expr>* ')' ;                         \
      expr     : <number> | <string> | <symbol> |          \
                 <sexpr> | <qexpr> | <comment>;            \
      lispy    : /^/ <expr>* /$/ ;                         \
    ",
    Number, Symbol, String, Comment, Qexpr, Sexpr, Expr, Lispy);

  // create environment and add functions
  lenv* env = lenv_new();
  lenv_add_builtins(env);

  // add some shortcuts too;
  lenv_def(env, lval_sym("true"),  lval_bool(1));
  lenv_def(env, lval_sym("false"), lval_bool(0));

  // iterate over files passed to us and load them before entering the repl...
  if (argc >= 2) {
    // first arg is the prog name
    for (int i = 1; i < argc; i++) {
      // let the mpc parser handle creating the string
      lval* filename = lval_add(lval_sexpr(), lval_str(argv[i]));

      // call load, which will will evaluate returning an lval
      lval* result = builtin_load(env, filename);

      // which we should spill out if an error, otherwise we just move on
      if (result->type == LVAL_ERR) {
        lval_println(result);
      }
      // release and remove on, lval_add
      lval_del(result);
    }
  }

  if (argc == 1) {

    int no_exit_signal = 1;

    // unless 'kill' has been passed, start the
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
  }

  lenv_del(env);

  /* Undefine and Delete our Parsers */
  mpc_cleanup(8, Number, Symbol, String, Comment, Qexpr, Sexpr, Expr, Lispy);

  return 0;
}
