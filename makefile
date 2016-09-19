client : client.c chat.h
	gcc -o client client.c -lpthread
server : server.c threadpool.c rb_tree.c threadpool.h rb_tree.h chat.h
	gcc -o server server.c threadpool.c rb_tree.c -lpthread
.PHONY :clean
clean  :
	rm -f server client
