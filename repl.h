struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };

typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
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

struct lenv {
  int count;
  char** syms;
  lval** vals;
};

#define LASSERT(args, condition, message, ...) \
  if (!(condition)) { \
    lval *err = lval_err(message, ##__VA_ARGS__); \
    lval_del(args); \
    return err; \
  }

// lisp-value generic instance operations
void  lval_del(lval* v);
lval* lval_add(lval* list, lval* incoming);
lval* lval_join(lval* x, lval* y);
lval* lval_pop(lval* expression, int index);
lval* lval_take(lval* val, int index);
lval* lval_copy(lval* org);

// instance types
lval* lval_num(long x);
lval* lval_err(char* message, ...);
lval* lval_sym(char* s);
lval* lval_qexpr(void);
lval* lval_sexpr(void);
lval* lval_fun(lbuiltin fn);

// environment instance operations
lenv* lenv_new(void);
lval* lenv_get(lenv* env, lval* key);
void  lenv_put(lenv* env, lval* key, lval* value);
void  lenv_del(lenv* env);

// lisp read and evaluation
lval* lval_eval(lenv* env, lval* val);
lval* lval_eval_sexpr(lenv* env, lval* expr);

// print utilities
void lval_print(lval* v);
void lval_println(lval* v);
void lval_expr_print(lval* v, char open, char close);
