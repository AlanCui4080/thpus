def:
	gcc -Werror *.c -DDEBUG && ./a.out
ndbg:
	gcc -Werror *.c && ./a.out
