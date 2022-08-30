def:
	gcc -Werror *.c -DDEBUG
ndbg:
	gcc -Werror *.c && ./a.out
tr:
	gcc -Werror *.c -DDEBUG -DDEBUG_TRACE && ./a.out
test:
	curl -v http://localhost:8888
	curl -v -r 0- http://localhost:8888
	curl -v -r 199- http://localhost:8888
	curl -v -r 0-199 http://localhost:8888
	curl -v -r 20-199 http://localhost:8888
	curl -v --HEAD http://localhost:8888
	curl -v -X OPTIONS http://localhost:8888
	curl -v -X TRACE http://localhost:8888
	echo "OK"
