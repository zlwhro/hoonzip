all: hoonzip hoonzip_sanitize

hoonzip: main.o decomp.o
	gcc -o hoonzip main.o decomp.o

main.o: main.c
	gcc -c main.c

decomp.o : decomp.c decomp.h
	gcc -c decomp.c  -march=native

hoonzip_sanitize: main_sanitize.o decomp_sanitize.o
	gcc -fsanitize=address -o hoonzip_sanitize main_sanitize.o decomp_sanitize.o

main_sanitize.o: main.c hoonzip.h
	gcc -fsanitize=address -o main_sanitize.o -c main.c

decomp_sanitize.o : decomp.c decomp.h
	gcc -fsanitize=address -o decomp_sanitize.o  -c decomp.c  -march=native

clean:
	rm *.o
	rm hoonzip
	rm hoonzip_sanitize
