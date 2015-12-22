repl:
	cc -std=c99 -Wall lvals.c utils.c types.c lib.c env.c repl.c mpc.c -ledit -lm -o repl
clean:
	$(RM) repl
