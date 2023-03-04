all:
	gcc -pthread -o lets-talk lets-talk.c list.c

clean:
	rm lets-talk

Valgrind:
	valgrind --leak-check=full --show-leak-kinds=all ./lets-talk 8008 127.0.0.1 8008