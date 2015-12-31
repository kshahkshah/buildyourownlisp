repl:
	make clean
	cc -std=c99 -Wall mpc.c lvals.c utils.c types.c lib.c env.c lispy.c -ledit -lm -o lispy
clean:
	$(RM) lispy
