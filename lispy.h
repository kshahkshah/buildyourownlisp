mpc_parser_t* Number; 
mpc_parser_t* Symbol; 
mpc_parser_t* String; 
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;  
mpc_parser_t* Qexpr;  
mpc_parser_t* Expr; 
mpc_parser_t* Lispy;

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

enum { LVAL_NUM, LVAL_BOOL, LVAL_ERR, LVAL_SYM, LVAL_STR,
       LVAL_FUN, LVAL_SIG, LVAL_SEXPR, LVAL_QEXPR };

typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
  // one of the enums, duh
  int type;

  // numbers
  long num;

  // boolean type, 0 or 1
  int boolean;

  // signals, should be 0-9
  int sig;

  // error messages
  char *err;

  // symbol references
  char *sym;

  // strings
  char *str;

  // builtin functions
  lbuiltin builtin;

  // user defined functions
  lenv* env;
  lval* formals;
  lval* body;

  // count is the list length of a s or q expression
  int count;

  // cell points to other lval pointers
  struct lval** cell;
};

struct lenv {
  lenv* parent;
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

// all in turn should call assert which will handle the
// variable number of arguments in formatting a message
#define LASSERT_TYPE(function, parent, index, expected) \
  LASSERT(parent, parent->cell[index]->type == expected, \
    "Function '%s', passed an unexpected type, you passed a %s at argument index %i when a %s was expected", \
    function, \
    lval_human_name(parent->cell[index]->type), \
    index, \
    lval_human_name(expected));

#define LASSERT_ARITY(function, arguments, expected) \
  LASSERT(arguments, arguments->count == expected, \
    "Function '%s', receive wrong number of arguments! %i for %i", \
    function, \
    arguments->count, \
    expected);

// parsing hooks
lval* lval_read_num(mpc_ast_t* tree);
lval* lval_read_str(mpc_ast_t* tree);
lval* lval_read(mpc_ast_t* tree);

// lisp-value generic instance operations
void  lval_del(lval* v);
lval* lval_add(lval* list, lval* incoming);
lval* lval_join(lval* x, lval* y);
lval* lval_pop(lval* expression, int index);
lval* lval_unshift(lval* list, lval* incoming);
lval* lval_take(lval* val, int index);
lval* lval_copy(lval* org);
int lval_true(lval* val);

// instance types
lval* lval_num(long x);
lval* lval_bool(int x);
lval* lval_sig(int x);
lval* lval_err(char* message, ...);
lval* lval_sym(char* s);
lval* lval_str(char* s);
lval* lval_qexpr(void);
lval* lval_sexpr(void);
lval* lval_fun(lbuiltin fn);
lval* lval_lambda(lval* formals, lval* body);

// environment instance operations
lenv* lenv_new(void);
lval* lenv_get(lenv* env, lval* key);
void  lenv_put(lenv* env, lval* key, lval* value);
void  lenv_def(lenv* env, lval* key, lval* value);
void  lenv_del(lenv* env);
lenv* lenv_copy(lenv* org);

void lenv_add_builtin(lenv* e, char* name, lbuiltin builtin);
void lenv_add_builtins(lenv* e) ;

// lisp read and evaluation
lval* lval_eval(lenv* env, lval* val);
lval* lval_eval_sexpr(lenv* env, lval* expr);

// print utilities
void lval_print(lval* v);
void lval_println(lval* v);
void lval_expr_print(lval* v, char open, char close);
char* lval_human_name(int t);
void lval_print_str(lval* v);
