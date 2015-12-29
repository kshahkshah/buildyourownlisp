#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpc.h"
#include "lispy.h"
#include "lib.h"

lenv* lenv_new(void) {
  lenv* env = malloc(sizeof(lenv));
  // just like cell
  env->count = 0;
  env->syms = NULL;
  env->vals = NULL;
  env->parent = NULL;

  return env;
}

lenv* lenv_copy(lenv* org) {
  lenv* dup = malloc(sizeof(lenv));

  // copy values and pointer to parent
  dup->count = org->count;
  dup->parent = org->parent;

  // allocate space for references and values
  dup->syms = malloc(sizeof(char*) * dup->count);
  dup->vals = malloc(sizeof(lval*) * dup->count);

  for(int i = 0; i < dup->count; i++) {
    // copy actual bytes here
    dup->syms[i] = malloc(strlen(org->syms[i]) + 1);
    strcpy(dup->syms[i], org->syms[i]);
    // we can lval_copy and get a pointer to the copied value
    dup->vals[i] = lval_copy(org->vals[i]);
  }

  return dup;
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

  // additionally now, crazy town as it is, check the parents environment, recursively
  if(env->parent) {
    return lenv_get(env->parent, key);
  } else {
    return lval_err("Unbound Symbol, there is no such function or reference '%s'", key->sym);
  }
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

// define a 'global' variable
void lenv_def(lenv* env, lval* key, lval* value) {
  if (env->parent) {
    lenv_def(env->parent, key, value);
  } else {
    lenv_put(env, key, value);
  }
}

// create the environment...

// straight copied and added too
// we're getting back copies of these lvals so free the ones passed in
void lenv_add_builtin(lenv* e, char* name, lbuiltin builtin) {
  lval* k = lval_sym(name);
  lval* v = lval_fun(builtin);
  lenv_put(e, k, v);
  lval_del(k); lval_del(v);
}

void lenv_add_builtins(lenv* e) {
  /* List Functions */
  lenv_add_builtin(e, "def",  builtin_def);
  lenv_add_builtin(e, "=",    builtin_put);
  lenv_add_builtin(e, "min",  builtin_min);
  lenv_add_builtin(e, "max",  builtin_max);

  lenv_add_builtin(e, ">", builtin_gt);
  lenv_add_builtin(e, "<", builtin_lt);
  lenv_add_builtin(e, ">=", builtin_gte);
  lenv_add_builtin(e, "<=", builtin_lte);
  lenv_add_builtin(e, "==", builtin_eq);
  lenv_add_builtin(e, "!=", builtin_neq);

  lenv_add_builtin(e, "if", builtin_if);
  lenv_add_builtin(e, "!", builtin_not);
  lenv_add_builtin(e, "&&", builtin_and);
  lenv_add_builtin(e, "||", builtin_or);

  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "join", builtin_join);
  lenv_add_builtin(e, "cons", builtin_cons);
  lenv_add_builtin(e, "length", builtin_length);
  lenv_add_builtin(e, "nth", builtin_nth);

  lenv_add_builtin(e, "quote", builtin_quote);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "exists", builtin_exists);
  lenv_add_builtin(e, "locals", builtin_locals);
  lenv_add_builtin(e, "type", builtin_type);
  lenv_add_builtin(e, "functions", builtin_functions);
  lenv_add_builtin(e, "exit", builtin_exit);
  lenv_add_builtin(e, "lambda", builtin_lambda);

  /* Mathematical Functions */
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
  lenv_add_builtin(e, "\%", builtin_mod);
  lenv_add_builtin(e, "^", builtin_exp);
}
