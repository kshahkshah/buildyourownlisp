lval* builtin_head(lenv* env, lval* a);
lval* builtin_join(lenv* env, lval* a);
lval* builtin_cons(lenv* env, lval* a);
lval* builtin_tail(lenv* env, lval* a);
lval* builtin_quote(lenv* env, lval* a);
lval* builtin_eval(lenv* env, lval* a);
lval* builtin_min(lenv* env, lval* a);
lval* builtin_max(lenv* env, lval* a);
lval* builtin_length(lenv* env, lval* a);

lval* builtin_locals(lenv* env, lval* a);
lval* builtin_functions(lenv* env, lval* a);

lval* builtin_exit(lenv* env, lval* a);
lval* builtin_exists(lenv* env, lval* a);

// comparisons
lval* builtin_gt(lenv* env, lval* a);
lval* builtin_lt(lenv* env, lval* a);

// control structures
lval* builtin_if(lenv* env, lval* a);

// defining a symbols
lval* builtin_def(lenv* e, lval* a);
lval* builtin_put(lenv* e, lval* a);
//___trigger the definition
lval* builtin_var(lenv* env, lval* args, char* op);

// define functions
lval* builtin_lambda(lenv* env, lval* a);

// math
lval* builtin_add(lenv* e, lval* a);
lval* builtin_sub(lenv* e, lval* a);
lval* builtin_mul(lenv* e, lval* a);
lval* builtin_div(lenv* e, lval* a);
lval* builtin_mod(lenv* e, lval* a);
lval* builtin_exp(lenv* e, lval* a);
//___triggers math
lval* builtin_op(lenv* e, lval* a, char* op);
