.PHONY:clean

main:main.o threads_pool.o
	gcc -lpthread main.o threads_pool.o -g -o main
main.o:main.c
	gcc -c main.c
threads_pool.o:threads_pool.c
	gcc -c -lpthread threads_pool.c
clean:
	rm ./main.o ./threads_pool.o
