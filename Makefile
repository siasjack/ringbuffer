all:
	gcc -o test -g test.c ringbuffer.c -lpthread
